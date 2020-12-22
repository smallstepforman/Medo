/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Media Source
 */
 
#ifndef _MEDIA_SOURCE_H_
#define _MEDIA_SOURCE_H_

#ifndef __STRING__
#include <string>
#endif

class BBitmap;
class BMediaFile;
class BMediaTrack;
class media_format;
class BString;

/*****************************
	MediaSource is a file on disk.
	(video, audio, video+audio or picture)
******************************/
	
class MediaSource
{
public:
	enum MEDIA_TYPE {MEDIA_INVALID, MEDIA_VIDEO, MEDIA_AUDIO, MEDIA_VIDEO_AND_AUDIO, MEDIA_PICTURE, NUMBER_MEDIA_TYPES};
	
					MediaSource(const char *filename);
					~MediaSource();
	void			CreateFileInfoString(BString *aString);
					
private:
	MEDIA_TYPE		fMediaType;
	std::string		fFilename;
	
	BBitmap			*fBitmap;
	BMediaFile		*fMediaFile;
	BMediaTrack 	*fVideoTrack;
	
	char			*fAudioBuffer;
	size_t			fAudioBufferSize;
	BMediaTrack		*fAudioTrack;

	void			CreateSecondaryMediaFile(const char *path);
	BMediaFile		*fSecondaryMediaFile;
	BMediaTrack		*fSecondaryVideoTrack;
	
	status_t		SetVideoTrack(const char *path, BMediaTrack *track, media_format *format);
	status_t		SetAudioTrack(const char *path, BMediaTrack *track, media_format *format);
	void			BuildVideoMediaFormat(BBitmap *bitmap, media_format *format);
	
	float			fVideoFieldRate;
	uint32			fInterlace;
	uint32			fVideoWidth;
	uint32			fVideoHeight;
	bigtime_t		fVideoDuration;
	int64			fVideoNumberFrames;
	
	bigtime_t		fAudioDuration;
	int64			fAudioNumberSamples;
	int32			fAudioSampleSize;
	int32			fAudioChannelCount;
	float			fAudioFrameRate;
	uint32			fAudioDataFormat;
	
//	Media source access functions	
public:
	const char			*GetFilename() const			{return fFilename.c_str();}
	const MEDIA_TYPE	GetMediaType() const			{return fMediaType;}

	BBitmap				*GetBitmap()					{return fBitmap;}
	BMediaTrack			*GetVideoTrack() const 			{return fVideoTrack;}
	BMediaTrack			*GetSecondaryVideoTrack() const	{return fSecondaryVideoTrack;}
	const uint32		GetVideoWidth() const			{return fVideoWidth;}
	const uint32		GetVideoHeight() const			{return fVideoHeight;}
	const float			GetVideoFrameRate() const		{return fVideoFieldRate;}
    const bigtime_t		GetVideoDuration() const		{return fVideoDuration;}
	const int64			GetVideoNumberFrames() const	{return fVideoNumberFrames;}
	
	BMediaTrack			*GetAudioTrack() const			{return fAudioTrack;}
	const bigtime_t		GetAudioDuration() const		{return fAudioDuration;}
	const int64			GetAudioNumberSamples() const	{return fAudioNumberSamples;}
	const int32			GetAudioNumberChannels() const	{return fAudioChannelCount;}
	const size_t		GetAudioSampleSize() const		{return fAudioSampleSize;}
	const float			GetAudioFrameRate() const		{return fAudioFrameRate;}
	char				*GetAudioBuffer() const			{return fAudioBuffer;}
	const size_t		GetAudioBufferSize() const		{return fAudioBufferSize;}
	const uint32		GetAudioDataFormat() const		{return fAudioDataFormat;}
};

#endif	//#ifndef _MEDIA_SOURCE_H_ 
