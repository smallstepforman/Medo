/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Project track
 */

#include <cstring>
#include <algorithm>

#include "EffectNode.h"
#include "Project.h"
#include "Language.h"

static int kTrackCreationIndex = 0;

/*	FUNCTION:		TimelineTrack :: TimelineTrack
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
TimelineTrack :: TimelineTrack()
	: mNumberEffectLayers(0), mVideoEnabled(true), mAudioEnabled(true)
{
	mAudioLevels[0] = 1.0f;
	mAudioLevels[1] = 1.0f;

	if (gProject->mTimelineTracks.empty())
		kTrackCreationIndex = 0;

	char buffer[32];
	sprintf(buffer, "%s#%lu", GetText(TXT_TIMELINE_TRACK), ++kTrackCreationIndex);
	mName.SetTo(buffer);
}

/*	FUNCTION:		TimelineTrack :: ~TimelineTrack
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
TimelineTrack :: ~TimelineTrack()
{
	for (auto i : mEffects)
		delete i;
}

/*	FUNCTION:		TimelineTrack :: AddClip
	ARGS:			clip
	RETURN:			clip index
	DESCRIPTION:	Add clip to track
*/
const int32 TimelineTrack :: AddClip(MediaClip &clip)
{
	int32 clip_index = 0;
	for (std::vector<MediaClip>::iterator i = mClips.begin(); i != mClips.end(); i++)
	{
		if (clip.mTimelineFrameStart < (*i).mTimelineFrameStart)
		{
			mClips.insert(i, clip);
			RepositionClips();
			return clip_index;
		}
		else
		{
			int64 end_point = (*i).GetTimelineEndFrame();
			if (clip.mTimelineFrameStart < end_point)
			{
				//	Get midpoint and determine if clip to be added before/after current clip
				int64 midpoint = (*i).mTimelineFrameStart + (*i).Duration()/2;
				if (midpoint < clip.mTimelineFrameStart)
				{
					//	midpoint before, insert after
					clip.mTimelineFrameStart = end_point;
					i++;
					if (i != mClips.end())
						mClips.insert(i, clip);
					else
						mClips.push_back(clip);
					clip_index++;
				}
				else
				{
					//	midpoint after, insert before
					(*i).mTimelineFrameStart = clip.mTimelineFrameStart + clip.Duration();
					mClips.insert(i, clip);
				}
				RepositionClips();
				return clip_index;
			}
		}
		clip_index++;
	}
	//	push back, no need to reposition
	mClips.push_back(clip);
	gProject->UpdateDuration();
	return clip_index;
}

/*	FUNCTION:		TimelineTrack :: RemoveClip
	ARGS:			clip
					remove_effects
	RETURN:			n/a
	DESCRIPTION:	Remove clip from track
*/
void TimelineTrack :: RemoveClip(MediaClip &clip, const bool remove_effects)
{
	for (std::vector<MediaClip>::iterator c = mClips.begin(); c != mClips.end(); c++)
	{
		if (clip == *c)
		{
			mClips.erase(c);

			if (remove_effects)
			{
				bool repeat;
				do
				{
					repeat = false;
					for (std::vector<MediaEffect *>::iterator e = mEffects.begin(); e != mEffects.end(); e++)
					{
						if ((((*e)->mTimelineFrameStart >= clip.mTimelineFrameStart) && ((*e)->mTimelineFrameStart < clip.GetTimelineEndFrame()))	&&
							((*e)->mTimelineFrameEnd <= clip.GetTimelineEndFrame()))
						{
							(*e)->mEffectNode->MediaEffectSelectedBase(nullptr);
							delete *e;
							mEffects.erase(e);
							repeat = true;
							break;
						}
					}
				} while (repeat);
			}
			break;
		}
	}
	gProject->UpdateDuration();
}

/*	FUNCTION:		TimelineTrack :: RepositionClips
	ARGS:			compact
	RETURN:			n/a
	DESCRIPTION:	Reposition clips so that they do not overlap
*/
void TimelineTrack :: RepositionClips(const bool compact)
{
	if (mClips.empty())
		return;

	if (compact)
		mClips[0].mTimelineFrameStart = 0;
	int64 end_pos = mClips[0].GetTimelineEndFrame();

	for (size_t i =1; i < mClips.size(); i++)
	{
		int64 clip_duration = mClips[i].Duration();
		if (compact)
		{
			mClips[i].mTimelineFrameStart = end_pos;
			end_pos += clip_duration;
		}
		else
		{
			if (mClips[i].mTimelineFrameStart < end_pos)
				mClips[i].mTimelineFrameStart = end_pos;
			end_pos = mClips[i].mTimelineFrameStart + clip_duration;
		}
	}
	gProject->UpdateDuration();
}

/*	FUNCTION:		TimelineTrack :: AddEffect
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Add effect to track
*/
void TimelineTrack :: SplitClip(MediaClip &clip, int64 frame_idx)
{
	MediaClip second_clip = clip;
	clip.mSourceFrameEnd = clip.mSourceFrameStart + frame_idx - clip.mTimelineFrameStart;
	second_clip.mSourceFrameStart = clip.mSourceFrameEnd;
	second_clip.mTimelineFrameStart = frame_idx;
	AddClip(second_clip);
}

/*	FUNCTION:		TimelineTrack :: AddEffect
	ARGS:			effect
	RETURN:			n/a
	DESCRIPTION:	Add effect to track
*/
void TimelineTrack :: AddEffect(MediaEffect *effect)
{
	mEffects.push_back(effect);
	SortEffects();

	//	Determine if effect layer conflict - if so, increase effect priority
	bool conflict;
	do
	{
		conflict = false;
		for (auto &i : mEffects)
		{
			if ((effect->mPriority == i->mPriority) && (effect != i))
			{
				MediaEffect *min = (effect->mTimelineFrameStart < i->mTimelineFrameStart) ? effect : i;
				MediaEffect *max = (min == effect ? i : effect);
				if (min->mTimelineFrameEnd >= max->mTimelineFrameStart)
				{
					effect->mPriority++;
					conflict = true;
					break;
				}
			}
		}
	} while (conflict);

	//	calculate number effect layers
	int32 highest_priority = 0;
	for (auto i : mEffects)
		if (i->mPriority > highest_priority)
			highest_priority = i->mPriority;
	mNumberEffectLayers = highest_priority + 1;
	gProject->UpdateDuration();
}

/*	FUNCTION:		TimelineTrack :: RemoveEffect
	ARGS:			effect
					free_memory
	RETURN:			n/a
	DESCRIPTION:	Remove effect from track
*/
void TimelineTrack :: RemoveEffect(MediaEffect *effect, const bool free_memory)
{
#if 0
	printf("TimelineTrack::RemoveEffect(%p, %d).  NumEffectsBefore = %ld\n", effect, free_memory, mEffects.size());
	for (auto i : mEffects)
		printf("%p %p %s\n", i, i->mEffectNode, i->mEffectNode->GetEffectName());
#endif

	for (std::vector<MediaEffect *>::iterator e = mEffects.begin(); e != mEffects.end(); e++)
	{
		if (effect == *e)
		{
			effect->mEffectNode->MediaEffectSelectedBase(nullptr);
			mEffects.erase(e);
			if (free_memory)
				delete effect;
			break;
		}
	}
	gProject->UpdateDuration();

	//	TODO may need to reduce effect priorities

#if 0
	printf("NumEffectsAfter = %ld\n", mEffects.size());
	for (auto i : mEffects)
		printf("%p %p %s\n", i, i->mEffectNode, i->mEffectNode->GetEffectName());
#endif
}

/*	FUNCTION:		TimelineTrack :: SortClips
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Sort timeline track
*/
void TimelineTrack :: SortClips()
{
	std::sort(mClips.begin(), mClips.end(), [](const MediaClip &a, const MediaClip &b){return (a.mTimelineFrameStart < b.mTimelineFrameStart);});
	gProject->UpdateDuration();
}

/*	FUNCTION:		TimelineTrack :: SortEffects
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Sort timeline track
*/
void TimelineTrack :: SortEffects()
{
	std::sort(mEffects.begin(), mEffects.end(), [](const MediaEffect *a, const MediaEffect *b){return (a->mTimelineFrameStart < b->mTimelineFrameStart);});
	gProject->UpdateDuration();
}

/*	FUNCTION:		TimelineTrack :: GetNumberEffects
	ARGS:			frame_idx
	RETURN:			number of effects
	DESCRIPTION:	Get number of effect layers at frame_idx
*/
const int32 TimelineTrack :: GetNumberEffects(int64 frame_idx)
{
	int32 number_effects = 0;
	for (auto &i : mEffects)
	{
		if ((frame_idx >= i->mTimelineFrameStart) && (frame_idx <= i->mTimelineFrameEnd))
			++number_effects;
	}
	return number_effects;
}

/*	FUNCTION:		TimelineTrack :: GetEffectIndex
	ARGS:			effect
	RETURN:			effect index
	DESCRIPTION:	Get effect index
*/
const int32 TimelineTrack :: GetEffectIndex(MediaEffect *effect)
{
	int32 effect_idx = 0;
	for (auto &i : mEffects)
	{
		if (effect == i)
			return effect_idx;
		++effect_idx;
	}
	return -1;
}

/*	FUNCTION:		TimelineTrack :: SetEffectPriority
	ARGS:			effect
					priority
	RETURN:			n/a
	DESCRIPTION:	Set effect priority.  Adjust neighbour effects
*/
void TimelineTrack :: SetEffectPriority(MediaEffect *effect, const int32 priority)
{
	bool lower = priority < effect->mPriority;
	effect->mPriority = priority;

	std::vector<MediaEffect *> layered_effects;
	for (auto &i : mEffects)
	{
		if (DoEffectsIntersect(i, effect))
			layered_effects.push_back(i);
	}

	//	Sort layers, dont create holes
	if (layered_effects.size() > 1)
	{
		std::sort(layered_effects.begin(), layered_effects.end(), [effect, lower](const MediaEffect *a, const MediaEffect *b)
			{
				if ((a == effect) && (a->mPriority == b->mPriority) && lower)
					return true;
				else if ((b == effect) && (a->mPriority == b->mPriority) && !lower)
					return true;
				else
					return (a->mPriority < b->mPriority);
			});
	}
	int start_layer = 0;
	for (auto &i : layered_effects)
	{
		i->mPriority = start_layer;
		++start_layer;
	}
}

/*	FUNCTION:		TimelineTrack :: DoEffectsIntersect
	ARGS:			a, b
	RETURN:			true if effects intersect
	DESCRIPTION:	Determine if tracks intersect
*/
const bool TimelineTrack :: DoEffectsIntersect(const MediaEffect *a, const MediaEffect *b)
{
	if ((a->mTimelineFrameStart <= b->mTimelineFrameEnd) && (a->mTimelineFrameEnd >= b->mTimelineFrameStart))
		return true;
	return false;
}
