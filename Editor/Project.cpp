/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Project data
 */

#include <cassert>
#include <cstring>

#include <support/SupportDefs.h>
#include <interface/Window.h>
#include <storage/FilePanel.h>
#include <interface/Bitmap.h>

#include "MediaSource.h"
#include "EffectNode.h"
#include "VideoManager.h"
#include "MedoWindow.h"
#include "TimelineView.h"
#include "EffectsManager.h"
#include "RenderActor.h"
#include "AudioMixer.h"

#include "Project.h"
#include "Project_Snapshot.h"

Project 		*gProject = nullptr;
VideoManager	*gVideoManager = nullptr;

/*	FUNCTION:		ProjectData :: Project
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Project :: Project()
{
	assert(gProject == nullptr);
	gProject = this;
	fMemento = nullptr;
	
	//	Defaults
#if 1
	mResolution.width = 1920;
	mResolution.height = 1080;
#else
	mResolution.width = 1280;
	mResolution.height = 720;
#endif
	mResolution.frame_rate = 30;
	
	assert(gVideoManager == nullptr);
	gVideoManager = new VideoManager;

	//	Add 2 tracks
	AddTimelineTrack(new TimelineTrack);
	AddTimelineTrack(new TimelineTrack);

	fMemento = new Memento;

	mTotalDuration = 0;
}

/*	FUNCTION:		ProjectData :: ~Project
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Project :: ~Project()
{
	assert(gProject != nullptr);

	delete fMemento;

	delete gVideoManager;		gVideoManager = nullptr;

	for (auto i : mMediaSources)
		delete i;
		
	for (auto i : mTimelineTracks)
		delete i;
		
	gProject = nullptr;
}

/*	FUNCTION:		Project :: InvalidatePreview
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Message to invalidate preview
*/
void Project :: InvalidatePreview()
{
	gRenderActor->Async(&RenderActor::AsyncPrepareFrame, gRenderActor, MedoWindow::GetInstance()->fTimelineView->GetCurrrentFrame());
}

/*	FUNCTION:		Project :: AddMediaSource
	ARGS:			source_file
					is_new
	RETURN:			media_source
	DESCRIPTION:	Add MediaSource to project
*/
MediaSource	* Project :: AddMediaSource(const char *source_file, bool &is_new)
{
	is_new = true;

	//	Check for duplicates
	for (auto i : mMediaSources)
	{
		if (i->GetFilename().Compare(source_file) == 0)
		{
			//printf("Project::AddMediaSource(%s) - duplicate\n", source_file);
			is_new = false;
			return i;
		}	
	}
	
	//	Is media source?
	MediaSource *source = new MediaSource(source_file);
	if (source->GetMediaType() != MediaSource::MEDIA_INVALID)
	{
		mMediaSources.push_back(source);
		return source;
	}
	else
	{
		delete source;
		return nullptr;	
	}	
}

/*	FUNCTION:		Project :: RemoveMediaSource
	ARGS:			source
	RETURN:			n/a
	DESCRIPTION:	Remove MediaSource from project
*/
void Project :: RemoveMediaSource(MediaSource *source)
{
	for (auto &t : mTimelineTracks)
	{
		bool repeat;
		do
		{
			repeat = false;
			for (std::vector<MediaClip>::iterator c = t->mClips.begin(); c != t->mClips.end(); c++)
			{
				if (c->mMediaSource == source)
				{
					t->mClips.erase(c);
					repeat = true;
					break;
				}
			}
		} while (repeat);
	}

	bool found = false;
	for (std::vector<MediaSource *>::iterator i = mMediaSources.begin(); i != mMediaSources.end(); i++)
	{
		if (source == *i)
		{
			mMediaSources.erase(i);
			delete source;
			found = true;
			break;	
		}	
	}
	if (!found)
		printf("Project::RemoveMediaSource() - not found\n");
}

/*	FUNCTION:		Project :: IsMediaSourceUsed
	ARGS:			source
	RETURN:			n/a
	DESCRIPTION:	Check if source actually used
					Used for SourceListView popup
*/
const bool Project :: IsMediaSourceUsed(MediaSource *source)
{
	bool used = false;
	for (auto &t : mTimelineTracks)
	{
		for (auto &c : t->mClips)
		{
			if (c.mMediaSource == source)
				return true;
		}
	}
	return false;
}

/*	FUNCTION:		Project :: AddTimelineTrack
	ARGS:			track
					index (-1 = end)
	RETURN:			n/a
	DESCRIPTION:	Add track to project
*/
void Project :: AddTimelineTrack(TimelineTrack *track, int index)
{
	if ((index >= 0) && (index < (int)mTimelineTracks.size()))
		mTimelineTracks.insert(mTimelineTracks.begin() + index, track);
	else
		mTimelineTracks.push_back(track);

	AudioMixer *audio_mixer = MedoWindow::GetInstance()->GetAudioMixer();
	if (audio_mixer)
		audio_mixer->PostMessage(AudioMixer::kMsgProjectInvalidated);
}

/*	FUNCTION:		Project :: AddTimelineTrack
	ARGS:			track
	RETURN:			n/a
	DESCRIPTION:	Add track to project
*/
void Project :: RemoveTimelineTrack(TimelineTrack *track)
{
	bool found = false;
	for (std::vector<TimelineTrack *>::iterator i = mTimelineTracks.begin(); i != mTimelineTracks.end(); i++)
	{
		if (track == *i)
		{
			mTimelineTracks.erase(i);
			delete track;
			found = true;
			break;	
		}	
	}
	if (!found)
		printf("Project::RemoveTimelineTrack() - not found\n");
}

/*	FUNCTION:		TimelineTrack :: GetTimelineTrackIndex
	ARGS:			track
	RETURN:			index
	DESCRIPTION:	Find track index
*/
int Project :: GetTimelineTrackIndex(TimelineTrack *track)
{
	int idx=0;
	for (auto &t : mTimelineTracks)
	{
		if (track == t)
			return idx;
		idx++;
	}
	return -1;
}

/*	FUNCTION:		TimelineTrack :: UpdateDuration
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Calculate total duration
*/
void Project :: UpdateDuration()
{
	mTotalDuration = 0;
	for (auto track : mTimelineTracks)
	{
		size_t num_clips = track->mClips.size();
		if (num_clips > 0)
		{
			//	Assumptions - mClips are sorted
			const MediaClip &clip = track->mClips[num_clips - 1];
			const int64 end_frame = clip.GetTimelineEndFrame();
			if (mTotalDuration < end_frame)
				mTotalDuration = end_frame;
		}

		size_t num_effects = track->mEffects.size();
		if (num_effects > 0)
		{
			//	Assumptions - mEffects are sorted
			const MediaEffect *effect = track->mEffects[num_effects - 1];
			if (mTotalDuration < effect->mTimelineFrameEnd)
				mTotalDuration = effect->mTimelineFrameEnd;
		}
	}
}

/*	FUNCTION:		Project :: CreateTimeString
	ARGS:			frame_idx
					buffer
					subsecond
	RETURN:			n/a
	DESCRIPTION:	Create sting format "0:24:30_200ms"
*/
void Project :: CreateTimeString(int64 frame_idx, char *buffer, const bool subsecond) const
{
	assert(buffer != nullptr);
	assert(frame_idx >= 0);
	
	const int64 kFramesHour = 60*60*kFramesSecond;
	const int64 kFramesMinute = 60*kFramesSecond;
	
	int64 hours = frame_idx/kFramesHour;
	frame_idx -= hours*kFramesHour;
	int64 min = frame_idx/kFramesMinute;
	frame_idx -= min*kFramesMinute;
	int64 sec = frame_idx/kFramesSecond;
	frame_idx -= sec*kFramesSecond;

	if (subsecond)
	{
		int64 frames = frame_idx/(kFramesSecond/mResolution.frame_rate);	//	Use 1-30 instead of 0-29
		if (hours == 0)
			sprintf(buffer, "%ldm:%02lds_%02ld", min, sec, frames);
		else
			sprintf(buffer, "%ldh:%02ldm:%02lds_%02ld", hours, min, sec, frames);
	}
	else
	{
		if (hours == 0)
			sprintf(buffer, "%ldm:%02lds", min, sec);
		else
			sprintf(buffer, "%ldh:%02ldm:%02lds", hours, min, sec);
	}
			
}

/*	FUNCTION:		Project :: DebugClips
	ARGS:			track_index
	RETURN:			n/a
	DESCRIPTION:	Display list of clips in track
*/
void Project :: DebugClips(const size_t track_index)
{
	if (track_index >= mTimelineTracks.size())
	{
		printf("DebugClips(%d > size(%d).  Listing all tracks\n", track_index, mTimelineTracks.size());
		for (size_t i=0; i < mTimelineTracks.size(); i++)
			DebugClips(i);	
	}
	printf("TimelineTrack[%d] clips:\n", track_index);
	TimelineTrack *track = mTimelineTracks[track_index];
	int clip_idx = 0;
	for (auto i : track->mClips)
	{
		printf("Clip[%d] TimelinePosStart[%ld]  FrameStart[%ld] FrameEnd[%ld] Duration[%ld] TimelinePosEnd[%ld] Source[%s]\n",
			clip_idx,
			i.mTimelineFrameStart,
			i.mSourceFrameStart,
			i.mSourceFrameEnd,
			i.Duration(),
			i.GetTimelineEndFrame(),
			i.mMediaSource->GetFilename().String());
		clip_idx++;
	}	
}

