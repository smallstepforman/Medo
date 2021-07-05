/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Media Source
 */

#include <cassert>

#if 0
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#endif

#include <MediaKit.h>
#include <StorageKit.h>

#include <interface/Bitmap.h>
#include <interface/Screen.h>
#include <translation/TranslationUtils.h>

#include "ImageUtility.h"
#include "VideoManager.h"
#include "MediaSource.h"
#include "MediaUtility.h"

/*	FUNCTION:		MediaSource :: MediaSource
	ARGS:			filename
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
MediaSource :: MediaSource(const char *filename)
	: fMediaType(MEDIA_INVALID), fMediaFile(nullptr), fVideoTrack(nullptr),
	  fAudioBuffer(nullptr), fAudioTrack(nullptr),
	  fSecondaryMediaFile(nullptr), fSecondaryVideoTrack(nullptr)
{
	assert(filename != nullptr);
		
	fFilename.SetTo(filename);
	
	//	Picture
	fBitmap = BTranslationUtils::GetBitmap(filename);
	if (fBitmap)
	{
		int32 bytes_per_row = fBitmap->BytesPerRow();
		int32 bits_length = fBitmap->BitsLength();
		fVideoWidth = bytes_per_row/4;
		fVideoHeight = bits_length/bytes_per_row;
		printf("Picture File (%dx%d)\n", fVideoWidth, fVideoHeight);
		fMediaType = MEDIA_PICTURE;
		return;
	}
	
	//	Media
	status_t	err;
	entry_ref	ref;
	err = get_ref_for_path(filename, &ref);
	if (err != B_NO_ERROR)
	{
		printf("MediaSource(%s) - Cannot open file (%d)\n", filename, err);
		return;
	}
	fMediaFile = new BMediaFile(&ref, B_MEDIA_FILE_BIG_BUFFERS);
	err = fMediaFile->InitCheck();
	if (err != B_OK)
	{
		printf("BMediaFile::InitCheck(%d)\n", err);
		delete fMediaFile;
		fMediaFile = nullptr;
		return;		
	}
	int32	num_tracks = fMediaFile->CountTracks();
	printf("MediaSource(%s) Found %d tracks\n", filename, num_tracks);
	for (int32 i = 0; i < num_tracks; i++)
	{
		printf("Processing track(%d)\n", i);
		BMediaTrack	*track = fMediaFile->TrackAt(i);
		if (track == nullptr)
		{
			fMediaFile->ReleaseAllTracks();
			printf("Media file claims to have %d tracks.", num_tracks);
			printf("Cannot find track(%d)\n", i);
			return;
		}
			
		media_format	mf;	//	MediaKit structure
		
		err = track->EncodedFormat(&mf);
		if (err == B_OK)
		{
			switch (mf.type)
			{
				case B_MEDIA_ENCODED_VIDEO:
				case B_MEDIA_RAW_VIDEO:
					err = SetVideoTrack(filename, track, &mf);
					break;
				case B_MEDIA_ENCODED_AUDIO:
				case B_MEDIA_RAW_AUDIO:
					err = SetAudioTrack(filename, track, &mf);
					break;
				default:
					err = B_ERROR;
			}
		}
		
		if (err != B_OK)
		{
			fMediaFile->ReleaseTrack(track);
			printf("Media has unrecognised format - track(%d)\n", i);
		}
	}
	
	if (fVideoTrack && fAudioTrack)
		fMediaType = MEDIA_VIDEO_AND_AUDIO;
	else if (fVideoTrack)
		fMediaType = MEDIA_VIDEO;
	else if (fAudioTrack)
		fMediaType = MEDIA_AUDIO;
	else
		printf("Unexpected error\n");

	if (fVideoTrack)
		CreateSecondaryMediaFile(filename);
}

/*	FUNCTION:		MediaSource :: ~MediaSource
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
MediaSource :: ~MediaSource()
{
	delete fBitmap;
	if (fVideoTrack)
	{
		assert(fMediaFile != nullptr);
		fMediaFile->ReleaseTrack(fVideoTrack);	
	}
	if (fAudioTrack)
	{
		assert(fMediaFile != nullptr);
		fMediaFile->ReleaseTrack(fAudioTrack);
		delete fAudioBuffer;
	}
	delete fMediaFile;

	//	Secondary video file
	if (fSecondaryVideoTrack)
	{
		assert(fSecondaryMediaFile != nullptr);
		fSecondaryMediaFile->ReleaseTrack(fSecondaryVideoTrack);
	}
	delete fSecondaryMediaFile;
}

/*	FUNCTION:		MediaSource :: SetVideoTrack
	ARGUMENTS:		path 	= filename of video
					track	= media file track
					format	= media format
	RETURN:			error code
	DESCRIPTION:	Prepare to decode video track
*/
status_t MediaSource :: SetVideoTrack(const char *path, BMediaTrack *track, media_format *format)
{
	media_codec_info	mci;
	track->GetCodecInfo(&mci);
	printf("   Video Decoder: %s\n", mci.pretty_name);

	if (fVideoTrack)
	{
		printf("   Multiple video tracks not supported (%s)\n", path);
		return B_ERROR;
	}
	
	//color_space bitmap_depth = BScreen().ColorSpace();
	color_space bitmap_depth = B_RGB32;
	
	BRect aRect(0.0, 0.0,
				format->u.encoded_video.output.display.line_width - 1.0,
				format->u.encoded_video.output.display.line_count - 1.0);
	fBitmap = new BBitmap(aRect, bitmap_depth);
	for(;;)
	{
		media_format mf, old_mf;

		BuildVideoMediaFormat(fBitmap, &mf);

		old_mf = mf;
		track->DecodedFormat(&mf);
				
		fVideoFieldRate = mf.u.raw_video.field_rate;
		fInterlace = mf.u.raw_video.interlace;
		fVideoWidth = mf.Width();
		fVideoHeight = mf.Height();
		
		//	check if match found
		if (old_mf.u.raw_video.display.format == mf.u.raw_video.display.format)
			break;
		
		printf("   SetVideoTrack: Colour space attempted: 0x%x, but it was reset to 0x%x\n",
			   bitmap_depth, mf.u.raw_video.display.format);
		
		bitmap_depth = mf.u.raw_video.display.format;
		delete fBitmap;
		fBitmap = new BBitmap(aRect, bitmap_depth);
		status_t st = fBitmap->InitCheck();
		if (st != B_OK)
		{
			printf("   SetVideoTrack()::fBitmap creation error: ");
			PrintErrorCode(st);
		}
	}
	
	fVideoTrack = track;
	fVideoDuration = track->Duration();
	fVideoNumberFrames = track->CountFrames();
		
	media_header mh;
	int64 dummy_num_frames = 0;
	track->ReadFrames((char *)fBitmap->Bits(), &dummy_num_frames, &mh);
	
	int32 bytes_per_row = fBitmap->BytesPerRow();
	int32 bits_length = fBitmap->BitsLength();
	printf("   Video File (%dx%d)\n", bytes_per_row/4, bits_length/bytes_per_row);
	printf("   Number frames = %ld\n", fVideoNumberFrames);
	printf("   Field rate = %f\n", fVideoFieldRate);
	printf("   Duration: %s\n", MediaDuration(fVideoDuration).Print());

	return B_OK;
}

/*	FUNCTION:		MediaSource :: BuildVideoMediaFormat
	ARGUMENTS:		bitmap
					format
	RETURN:			none
	DESCRIPTION:	Set media to comply with default format
					Moved to seperate function to aid readability 
*/
void MediaSource :: BuildVideoMediaFormat(BBitmap *bitmap, media_format *format)
{
	media_raw_video_format *rvf = &format->u.raw_video;

	memset(format, 0, sizeof(media_format));

	BRect bitmapBounds = bitmap->Bounds();

	rvf->last_active = (uint32)(bitmapBounds.Height() - 1.0);
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = 1;
	rvf->pixel_height_aspect = 3;
	rvf->display.format = bitmap->ColorSpace();
	rvf->display.line_width = (int32)bitmapBounds.Width();
	rvf->display.line_count = (int32)bitmapBounds.Height();
	rvf->display.bytes_per_row = bitmap->BytesPerRow();
}

/*	FUNCTION:		MediaSource :: SetAudioTrack
	ARGS:			path
					track
					format
	RETURN:			status
	DESCRIPTION:	Parse audio track
*/
status_t MediaSource :: SetAudioTrack(const char *path, BMediaTrack *track, media_format *format)
{
	media_codec_info	mci;
	track->GetCodecInfo(&mci);
	printf("   Audio Decoder: %s\n", mci.pretty_name);

	if (fAudioTrack)
	{
		printf("   Multiple audio tracks not supported (%s)\n", path);
		return B_ERROR;
	}

	fAudioTrack = track;
	fAudioNumberSamples = track->CountFrames();
	fAudioDuration = track->Duration();

	//	Attemp B_AUDIO_FLOAT audio format
	media_format mf;
	memset(&mf, 0, sizeof(mf));
	mf.type = B_MEDIA_RAW_AUDIO;
	mf.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	status_t err = track->DecodedFormat(&mf);
	if (err)
	{
		printf("   MediaSource::SetAudioTrack::DecodedFormat error (%s) %s\n", path, strerror(err));
		return B_ERROR;
	}

	fAudioSampleSize = mf.AudioFrameSize();
	fAudioChannelCount = mf.u.raw_audio.channel_count;
	fAudioFrameRate = mf.u.raw_audio.frame_rate;
	fAudioDataFormat = mf.u.raw_audio.format;
	fAudioBufferSize = mf.u.raw_audio.buffer_size;
	fAudioBuffer = new char [fAudioBufferSize];

	media_header mh;
	int64 dummy_num_frames = 0;
	if (!gVideoManager->LockMediaKit())
		return B_ERROR;
	err = track->ReadFrames(fAudioBuffer, &dummy_num_frames, &mh);
	gVideoManager->UnlockMediaKit();
	if (err)
	{
		printf("   MediaSource::SetAudioTrack::ReadFrames error (%s) %s\n", path, strerror(err));
		return B_ERROR;
	}


	switch (fAudioDataFormat)
	{
		case media_raw_audio_format :: B_AUDIO_UCHAR:		printf("   AudioDataFormat B_AUDIO_UCHAR\n");		break;
		case media_raw_audio_format :: B_AUDIO_SHORT:		printf("   AudioDataFormat B_AUDIO_SHORT\n");		break;
		case media_raw_audio_format :: B_AUDIO_INT:			printf("   AudioDataFormat B_AUDIO_INT\n");			break;
		case media_raw_audio_format :: B_AUDIO_FLOAT:		printf("   AudioDataFormat B_AUDIO_FLOAT\n");		break;
		default:											printf("   AudioDataFormat Unknown(%0x)\n", fAudioDataFormat);	return B_ERROR;
	}
	
	//printf("   buffer_size = %lu\n", fAudioBufferSize);
	printf("   num_samples = %ld\n", fAudioNumberSamples);
	printf("   track_duration = %ld\n", fAudioDuration);
	printf("   sample_size = %u\n", fAudioSampleSize);
	printf("   Channel count = %u\n", fAudioChannelCount);
	printf("   Frame rate = %f\n", fAudioFrameRate);
	printf("   Duration: %s\n", MediaDuration(fAudioDuration).Print());

	if (!fBitmap)
		fBitmap = BTranslationUtils::GetBitmap("Resources/icon_pcm.png");
	
	return B_OK;	
}

/*	FUNCTION:		MediaSource :: CreateSecondaryMediaFile
	ARGS:			path
	RETURN:			n/a
	DESCRIPTION:	Secondary video file absorbs seek costs when accessing the same media file.
					This is primarily used for video track thumbnails
*/
void MediaSource :: CreateSecondaryMediaFile(const char *filename)
{
	status_t	err;
	entry_ref	ref;
	err = get_ref_for_path(filename, &ref);
	if (err != B_NO_ERROR)
	{
		printf("MediaSource::CreateSecondaryMediaFile() - Cannot open file (%d)\n", err);
		return;
	}
	fSecondaryMediaFile = new BMediaFile(&ref, B_MEDIA_FILE_BIG_BUFFERS);
	err = fSecondaryMediaFile->InitCheck();
	if (err != B_OK)
	{
		printf("BMediaFile::InitCheck2(%d)\n", err);
		delete fSecondaryMediaFile;
		fSecondaryMediaFile = nullptr;
		return;
	}
	int32	num_tracks = fSecondaryMediaFile->CountTracks();
	for (int32 i = 0; i < num_tracks; i++)
	{
		BMediaTrack	*track = fSecondaryMediaFile->TrackAt(i);
		if (track == nullptr)
		{
			fSecondaryMediaFile->ReleaseAllTracks();
			printf("Media file claims to have %d tracks.", num_tracks);
			printf("Cannot find track(%d)\n", i);
			return;
		}

		media_format	mf;	//	MediaKit structure

		err = track->EncodedFormat(&mf);
		if (err == B_OK)
		{
			switch (mf.type)
			{
				case B_MEDIA_ENCODED_VIDEO:
				case B_MEDIA_RAW_VIDEO:
				{
					fSecondaryVideoTrack = track;	//	All sanity checks already performed with SetVideoTrack()
					media_format mf;
					BuildVideoMediaFormat(fBitmap, &mf);
					track->DecodedFormat(&mf);
					return;
				}
				case B_MEDIA_ENCODED_AUDIO:
				case B_MEDIA_RAW_AUDIO:
					fSecondaryMediaFile->ReleaseTrack(track);
					break;
				default:
					err = B_ERROR;
			}
		}

		if (err != B_OK)
			fSecondaryMediaFile->ReleaseTrack(track);
	}
	
	//	We should never be here
	assert(0);
}

/*	FUNCTION:		MediaSource :: CreateFileInfoString
	ARGS:			aString
	RETURN:			via aString
	DESCRIPTION:	Populate File Info string
*/
void MediaSource :: CreateFileInfoString(BString *aString)
{
	aString->SetTo(fFilename);

	if (fVideoTrack)
	{
		MediaDuration		duration(GetVideoDuration());
		BString				video_text;
		media_codec_info	mci;
		fVideoTrack->GetCodecInfo(&mci);
		video_text.SetToFormat("\n\nVideo Duration: %s\nResolution: %d x %d\nFrame Rate: %0.3f fps\nNumber Frames: %ld\nCodec: %s\n",
										duration.Print(),
										GetVideoWidth(),
										GetVideoHeight(),
										GetVideoFrameRate(),
										GetVideoNumberFrames(),
										mci.pretty_name);
		aString->Append(video_text);
	}
	if (fAudioTrack)
	{
		MediaDuration duration(GetAudioDuration());
		BString audio_text;
		media_codec_info	mci;
		fAudioTrack->GetCodecInfo(&mci);
		audio_text.SetToFormat("\n\nAudio Duration: %s\nFrame rate: %0.2f bps\nNumber Channels: %d\nNumber Samples: %ld\nCodec: %s\n",
										duration.Print(),
										GetAudioFrameRate(),
										GetAudioNumberChannels(),
										GetAudioNumberSamples(),
										mci.pretty_name);
		aString->Append(audio_text);
	}
	if (fMediaType == MEDIA_PICTURE)
	{
		BString picture_text;
		picture_text.SetToFormat("\n\nSize: %d x %d\n", (int32)GetVideoWidth(), (int32)GetVideoHeight());
		aString->Append(picture_text);
	}
}

void MediaSource :: SetLabel(const char *label)
{
	fLabel.SetTo(label);
}

const bigtime_t MediaSource :: GetTotalDuration() const
{
	return MAX(fVideoDuration, fAudioDuration);
}
