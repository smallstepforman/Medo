/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Project data
 */
 
#ifndef _PROJECT_H_
#define _PROJECT_H_

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

#ifndef _B_STRING_H
#include <support/String.h>
#endif

#ifndef _MEDIA_SOURCE_H_
#include "MediaSource.h"
#endif

class VideoManager;
class MedoWindow;
class MediaSource;
class EffectNode;
class Memento;

class BBitmap;
class BMessage;

/*****************************
	Timeline clip data
	- media source
	- source track start/end
	- timeline location
	Frame = bigtime_t unit (1 microsecond) 
******************************/
class MediaClip
{
public:
					MediaClip() : mMediaSource(nullptr), mVideoEnabled(true), mAudioEnabled(true) {}
					MediaClip(MediaSource *source) : mMediaSource(source) {}
					
	//	Source info
	MediaSource					*mMediaSource;					//	Media source index
	uint32						mMediaSourceType;				//	Track type when source has multiple tracks
	bigtime_t					mSourceFrameStart;				//	start position
	bigtime_t					mSourceFrameEnd;				//	end position
	inline const bigtime_t		Duration() const				{return mSourceFrameEnd - mSourceFrameStart;}
	inline const bigtime_t		GetTimelineEndFrame() const		{return mTimelineFrameStart + Duration();}

	bool						mVideoEnabled;
	bool						mAudioEnabled;
	
	//	Timeline track info
	bigtime_t					mTimelineFrameStart;			//	Timeline frame index
	BString						mTag;
	
	//	Operator
	inline bool operator ==(const MediaClip &a) 
	{
		return ((a.mMediaSource 		==	mMediaSource) &&
				(a.mMediaSourceType		==	mMediaSourceType) &&
				(a.mSourceFrameStart	==	mSourceFrameStart) &&
				(a.mSourceFrameEnd		==	mSourceFrameEnd) &&
				(a.mTimelineFrameStart	==	mTimelineFrameStart));
	} 
};

/*****************************
	MediaEffect base class
	Effects can be image (GLSL) or audio
******************************/
class MediaEffect
{
public:
	enum MEDIA_EFFECT {MEDIA_EFFECT_IMAGE, MEDIA_EFFECT_AUDIO};
	
							MediaEffect() : mEffectNode(nullptr), mEffectData(nullptr), mPriority(0), mEnabled(true) {}
	virtual					~MediaEffect() {}
	virtual MEDIA_EFFECT	Type() const = 0;
	
	EffectNode				*mEffectNode;
	void					*mEffectData;	//	Each Derived effect must delete effect data
	int32					mPriority;
	bigtime_t				mTimelineFrameStart;
	bigtime_t				mTimelineFrameEnd;
	bool					mEnabled;
	inline const bigtime_t	Duration() const	{return mTimelineFrameEnd - mTimelineFrameStart;}
};

//	Image Media Effect
class ImageMediaEffect : public MediaEffect
{
public:
	virtual					~ImageMediaEffect() {}
	MEDIA_EFFECT			Type() const override {return MEDIA_EFFECT_IMAGE;}
};

//	Audio Media Effect
class AudioMediaEffect : public MediaEffect
{
public:
	virtual					~AudioMediaEffect() {}
	MEDIA_EFFECT			Type() const override {return MEDIA_EFFECT_AUDIO;}
};

/*****************************
	Notes
******************************/
class MediaNote
{
public:
	bigtime_t			mTimelineFrame;
	BString				mText;
	float				mWidth;
	float				mHeight;

	std::vector<float>	mTextWidths;
};

/*****************************
	Timeline track, a collection of MediaClip's
******************************/
class TimelineTrack
{
public:
	std::vector<MediaClip>		mClips;
	std::vector<MediaEffect *>	mEffects;
	std::vector<MediaNote>		mNotes;
	int32						mNumberEffectLayers;
	bool						mVideoEnabled;
	bool						mAudioEnabled;
	float						mAudioLevels[2];
	BString						mName;

					TimelineTrack();
					~TimelineTrack();
	const int32		AddClip(MediaClip &clip);
	void			RemoveClip(MediaClip &clip, const bool remove_effects);
	void			RepositionClips(const bool compact = false);
	void			SplitClip(MediaClip &clip, int64 frame_idx);
	void			AddEffect(MediaEffect *effect);
	void			RemoveEffect(MediaEffect *effect, const bool free_memory=true);
	void			SortClips();
	void			SortEffects();
	const int32		GetNumberEffects(int64 frame_idx);
	const int32		GetEffectIndex(MediaEffect *effect);
	void			SetEffectPriority(MediaEffect *effect, const int32 priority);

private:
	const bool		DoEffectsIntersect(const MediaEffect *a, const MediaEffect *b);
};
	

/*****************************
	Project data, which can be loaded/saved
******************************/
class Project
{
public:
				Project();
				~Project();
				
	const bool	LoadProject(const char *data, const bool clear_media);
	const bool	SaveProject(FILE *output_file);
	
	MediaSource	*AddMediaSource(const char *source_file, bool &is_new);
	void		RemoveMediaSource(MediaSource *source);
	const bool	IsMediaSourceUsed(MediaSource *source);
	
	void		AddTimelineTrack(TimelineTrack *, int index=-1);
	void		RemoveTimelineTrack(TimelineTrack *);
	int			GetTimelineTrackIndex(TimelineTrack *);
				
	std::vector<MediaSource *>		mMediaSources;
	std::vector<TimelineTrack *>	mTimelineTracks;
	
	struct RESOLUTION
	{
		unsigned int		width;
		unsigned int		height;
		float				frame_rate;
	};
	RESOLUTION	mResolution;
	
	void			InvalidatePreview();

//	Undo support
	void			Snapshot();
	void			Undo();
	void			Redo();
	void			ResetSnapshots(const bool can_snap);
	Memento			*fMemento;

//	Duration
	void			UpdateDuration();
	int64			mTotalDuration;

//	Utility
	void			CreateTimeString(const bigtime_t frame_idx, char *buffer, const bool subsecond = false) const;
	
//	Diagnostic
	void		DebugClips(const size_t track_index);
};

//	Global project instance
extern Project 			*gProject;
extern VideoManager		*gVideoManager;

constexpr bigtime_t		kFramesSecond = 1'000'000;

#endif	//#ifndef _PROJECT_H_ 
