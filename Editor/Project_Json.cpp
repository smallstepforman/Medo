/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Project data
 */


#include <cstdio>
#include <cstdlib>
#include <vector>

#include <support/String.h>
#include <support/SupportDefs.h>
#include <interface/Alert.h>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "Project.h"
#include "FileUtility.h"
#include "MediaSource.h"
#include "MedoWindow.h"
#include "TimelineView.h"
#include "EffectsManager.h"
#include "EffectNode.h"
#include "RenderActor.h"

struct SOURCE
{
	unsigned int 	id;
	MediaSource::MEDIA_TYPE		type;
	std::string		filename;
};
static const char *kMediaType[] = {"invalid", "video", "audio", "video_audio", "image"};
static_assert(sizeof(kMediaType)/sizeof(char *) == MediaSource::NUMBER_MEDIA_TYPES, "sizeof(kMediaType) != MediaSource::NUMBER_MEDIA_TYPES");

struct CLIP
{
	unsigned int	id;
	unsigned int	source;
	int64			start;
	int64			end;
	int64			timeline;
	BString			tag;
	bool			video_enabled;
	bool			audio_enabled;
};

struct EFFECT
{
	unsigned int	id;
	MediaEffect		*media_effect;
};

struct NOTE
{
	unsigned int	id;
	int64			timeline;
	BString			text;
};

struct TRACK
{
	unsigned int	id;
	bool			global;
	bool			video_enabled;
	bool			audio_enabled;
	float			audio_levels[2];
	std::vector<unsigned int>	clips;
	std::vector<unsigned int>	effects;
	std::vector<unsigned int>	notes;
};

/*	FUNCTION:		LoadProject
	ARGUMENTS:		data
					clear_media
	RETURN:			true if successful
	DESCRIPTION:	Parse "*.medo" json file
*/
const bool Project :: LoadProject(const char *data, const bool clear_media)
{
	RESOLUTION				inResolution;
	std::vector<SOURCE>		inSources;
	std::vector<CLIP>		inClips;
	std::vector<EFFECT>		inEffects;
	std::vector<NOTE>		inNotes;
	std::vector<TRACK>		inTracks;
	TimelineView::SESSION	inSession;

#define ERROR_EXIT(a) {printf("Project::LoadProject() Error(%s)\n", a);	return false;}
	
	rapidjson::Document document;
	rapidjson::ParseResult res = document.Parse<rapidjson::kParseTrailingCommasFlag>(data);
	if (!res)
	{
		printf("JSON parse error: %s (Byte offset: %lu)\n", rapidjson::GetParseError_En(res.Code()), res.Offset());
		
		for (int c=-20; c < 20; c++)
			printf("%c", data[res.Offset() + c]);
		printf("\n");
		
		ERROR_EXIT("Invalid JSON");
		
	}
	//if (!document.IsObject())
	//	ERROR_EXIT("Invalid JSON");
	
	//	"medo" (header)
	{
		if (!document.HasMember("medo"))
			ERROR_EXIT("Missing object \"medo\"");
		const rapidjson::Value &header = document["medo"];
		if (!header.HasMember("version") || !header["version"].IsInt())
			ERROR_EXIT("Missing attribute medo::version");
		int version = header["version"].GetInt();
		if (version != 1)
			ERROR_EXIT("medo::version != 1");
	}
	
	//	"resolution"
	{
		if (!document.HasMember("resolution"))
			ERROR_EXIT("Missing object \"resolution\"");
		const rapidjson::Value &resolution = document["resolution"];
		if (!resolution.HasMember("width") || !resolution["width"].IsUint())
			ERROR_EXIT("Missing attribute resolution::width");
		inResolution.width = resolution["width"].GetUint();
		if (!resolution.HasMember("height") || !resolution["height"].IsUint())
			ERROR_EXIT("Missing attribute resolution::height");
		inResolution.height = resolution["height"].GetUint();
		if (!resolution.HasMember("frame_rate") || !resolution["frame_rate"].IsFloat())
			ERROR_EXIT("Missing attribute resolution::frame_rate");
		inResolution.frame_rate = resolution["frame_rate"].GetFloat();
	}
	
	//	"sources"
	{
		if (!document.HasMember("sources"))
			ERROR_EXIT("Missing object \"sources\"");
		const rapidjson::Value &sources = document["sources"];
		if (!sources.IsArray())
			ERROR_EXIT("\"sources\" is not an array");
		for (auto &v : sources.GetArray())
		{
			SOURCE aSource;
			//	"id"
			if (!v.HasMember("id") || !v["id"].IsUint())
				ERROR_EXIT("Missing attribute source::id");
			aSource.id = v["id"].GetUint();
			bool valid_id = true;
			for (auto i : inSources)
			{
				if (aSource.id == i.id)
					valid_id = false;
			}
			if (!valid_id)
				ERROR_EXIT("Duplicate sources::id");
			//	"type"
			if (!v.HasMember("type") || !v["type"].IsString())
				ERROR_EXIT("Missing attribute sources::type");
			const char *type = v["type"].GetString();
			bool valid_source_type = false;
			for (size_t i=0; i < sizeof(kMediaType)/sizeof(char *); i++)
			{
				if (strcmp(type, kMediaType[i]) == 0)
				{
					aSource.type = (MediaSource::MEDIA_TYPE) i;
					valid_source_type = true;
					break;
				}
			}
			if (!valid_source_type)
				ERROR_EXIT("Invalid sources::type");
			//	"file"
			if (!v.HasMember("file") || !v["file"].IsString())
				ERROR_EXIT("Missing attribute sources::file");
			aSource.filename.assign(v["file"].GetString());

			FILE *file = fopen(aSource.filename.c_str(), "rb");
			if (!file)
			{
				BAlert *alert = new BAlert("Invisible title", aSource.filename.c_str(), "File not found");
				alert->Go();
				ERROR_EXIT("Attribute sources::file");
			}
			fclose(file);
			
			inSources.push_back(aSource);
		}
	}
	
	//	"clips"
	{
		if (!document.HasMember("clips"))
			ERROR_EXIT("Missing object \"clips\"");
		const rapidjson::Value &clips = document["clips"];
		if (!clips.IsArray())
			ERROR_EXIT("\"clips\" is not an array");
		for (auto &v : clips.GetArray())
		{
			CLIP aClip;
			//	"id"
			if (!v.HasMember("id") || !v["id"].IsUint())
				ERROR_EXIT("Missing attribute clips::id");
			aClip.id = v["id"].GetUint();
			bool valid_id = true;
			for (auto i : inClips)
			{
				if (aClip.id == i.id)
					valid_id = false;
			}
			if (!valid_id)
				ERROR_EXIT("Duplicate clips::id");
			//	"source"
			if (!v.HasMember("source") || !v["source"].IsUint())
				ERROR_EXIT("Missing attribute clips::source");
			aClip.source = v["source"].GetUint();
			bool valid_source = false;
			for (auto &source : inSources)
			{
				if (aClip.source == source.id)
				{
					valid_source = true;
					break;
				}
			}
			if (!valid_source)
				ERROR_EXIT("Clip refers to invalid source");
			//	"start"
			if (!v.HasMember("start") || !v["start"].IsInt64())
				ERROR_EXIT("Missing attribute clips::start");
			aClip.start = v["start"].GetInt64();
			//	"end"
			if (!v.HasMember("end") || !v["end"].IsInt64())
				ERROR_EXIT("Missing attribute clips::end");
			aClip.end = v["end"].GetInt64();
			//	"timeline"
			if (!v.HasMember("timeline") || !v["timeline"].IsInt64())
				ERROR_EXIT("Missing attribute clips::timeline");
			aClip.timeline = v["timeline"].GetInt64();
			//	"video_enabled"
			if (!v.HasMember("video_enabled") || (!v["video_enabled"].IsBool()))
				ERROR_EXIT("Missing attribute clips::video_enabled");
			aClip.video_enabled = v["video_enabled"].GetBool();
			//	"audio_enabled"
			if (!v.HasMember("audio_enabled") || (!v["audio_enabled"].IsBool()))
				ERROR_EXIT("Missing attribute clips::audio_enabled");
			aClip.audio_enabled = v["audio_enabled"].GetBool();
			//	"tag"
			if (v.HasMember("tag"))
			{
				if (!v["tag"].IsString())
					ERROR_EXIT("Clip has invalid tag");
				aClip.tag.SetTo(v["tag"].GetString());
			}

			inClips.push_back(aClip);
		}
	}
	
	//	"effects"
	{
		if (!document.HasMember("effects"))
			ERROR_EXIT("Missing object \"effects\"");
		const rapidjson::Value &effects = document["effects"];
		if (!effects.IsArray())
			ERROR_EXIT("\"effects\" is not an array");
		for (auto &v : effects.GetArray())
		{
			EFFECT anEffect;
			
			//	"id"
			if (!v.HasMember("id") || !v["id"].IsUint())
				ERROR_EXIT("Missing attribute effects::id");
			anEffect.id = v["id"].GetInt();
			bool valid_id = true;
			for (auto i : inEffects)
			{
				if (anEffect.id == i.id)
					valid_id = false;
			}
			if (!valid_id)
				ERROR_EXIT("Duplicate effects::id");
			//	"vendor"
			if (!v.HasMember("vendor") || !v["vendor"].IsString())
				ERROR_EXIT("Missing attribute effects::vendor");
			std::string effect_vendor(v["vendor"].GetString());
			//	"name"
			if (!v.HasMember("name") || !v["name"].IsString())
				ERROR_EXIT("Missing attribute effects::name");
			std::string effect_name(v["name"].GetString());
			
			anEffect.media_effect = gEffectsManager->CreateMediaEffect(effect_vendor.c_str(), effect_name.c_str());

			if (anEffect.media_effect)
			{
				//	"start"
				if (!v.HasMember("start") || !v["start"].IsInt64())
					ERROR_EXIT("Missing attribute effects::start");
				anEffect.media_effect->mTimelineFrameStart = v["start"].GetInt64();
				//	"end"
				if (!v.HasMember("end") || !v["end"].IsInt64())
					ERROR_EXIT("Missing attribute effects::end");
				anEffect.media_effect->mTimelineFrameEnd = v["end"].GetInt64();
				//	"priority"
				if (!v.HasMember("priority") || !v["priority"].IsInt())
					ERROR_EXIT("Missing attribute effects::priority");
				anEffect.media_effect->mPriority = v["priority"].GetInt();
				//	"enabled"
				if (!v.HasMember("enabled") || !v["enabled"].IsBool())
					ERROR_EXIT("Missing attribute effects::enabled");
				anEffect.media_effect->mEnabled = v["enabled"].GetBool();
				//	"parameters"
				if (v.HasMember("parameters"))
				{
					anEffect.media_effect->mEffectNode->LoadParameters(v["parameters"], anEffect.media_effect);
					//	TODO
					//anEffect.media_effect  has_parameters = false;
				}
				else
					;//anEffect.has_parameters = false;
			
				inEffects.push_back(anEffect);
			}
		}
	}

	//	"notes"
	{
		if (!document.HasMember("notes"))
			ERROR_EXIT("Missing object \"notes\"");
		const rapidjson::Value &notes = document["notes"];
		if (!notes.IsArray())
			ERROR_EXIT("\"notes\" is not an array");
		for (auto &v : notes.GetArray())
		{
			NOTE aNote;

			//	"id"
			if (!v.HasMember("id") || !v["id"].IsUint())
				ERROR_EXIT("Missing attribute notes::id");
			aNote.id = v["id"].GetInt();
			bool valid_id = true;
			for (auto i : inNotes)
			{
				if (aNote.id == i.id)
					valid_id = false;
			}
			if (!valid_id)
				ERROR_EXIT("Duplicate notes::id");
			//	"timeline"
			if (!v.HasMember("timeline") || !v["timeline"].IsInt64())
				ERROR_EXIT("Missing attribute notes::timeline");
			aNote.timeline = v["timeline"].GetInt64();
			//	"text"
			if (!v.HasMember("text") || !v["text"].IsString())
				ERROR_EXIT("Missing attribute notes::text");
			aNote.text.SetTo(v["text"].GetString());

			inNotes.push_back(aNote);
		}
	}
	
	//	"tracks"
	{
		if (!document.HasMember("tracks"))
			ERROR_EXIT("Missing object \"tracks\"");
		const rapidjson::Value &tracks = document["tracks"];
		if (!tracks.IsArray())
			ERROR_EXIT("\"tracks\" is not an array");
		for (auto &v : tracks.GetArray())
		{
			TRACK aTrack;
			//	"id"
			if (!v.HasMember("id") || !v["id"].IsUint())
				ERROR_EXIT("Missing attribute tracks::id");
			aTrack.id = v["id"].GetInt();
			bool valid_id = true;
			for (auto i : inTracks)
			{
				if (aTrack.id == i.id)
					valid_id = false;
			}
			if (!valid_id)
				ERROR_EXIT("Duplicate tracks::id");
			//	"global"
			if (v.HasMember("global"))
			{
				if (!v["global"].IsBool())
					ERROR_EXIT("Corrupt attribute tracks::global");
				aTrack.global = v["global"].GetBool();
			}
			else
				aTrack.global = false;

			//	"video_enabled"
			if (v.HasMember("video_enabled") && v["video_enabled"].IsBool())
				aTrack.video_enabled = v["video_enabled"].GetBool();
			else
				ERROR_EXIT("Corrupt attribute tracks::video_enabled");
			//	"audio_enabled"
			if (v.HasMember("audio_enabled") && v["audio_enabled"].IsBool())
				aTrack.audio_enabled = v["audio_enabled"].GetBool();
			else
				ERROR_EXIT("Corrupt attribute tracks::video_enabled");

			//	levels
			if (!v.HasMember("levels") || !v["levels"].IsArray())
				ERROR_EXIT("track::\"levels\" is not an array");
			const rapidjson::Value &levels = v["levels"];
			if (levels.GetArray().Size() != 2)
				ERROR_EXIT("track::\"levels\" size must be 2");
			for (rapidjson::SizeType lvl=0; lvl < levels.GetArray().Size(); lvl++)
			{
				if (!levels.GetArray()[lvl].IsFloat())
					ERROR_EXIT("Invalid track::\"levels\" member (must be float)");
				aTrack.audio_levels[lvl] = levels.GetArray()[lvl].GetFloat();
				if (aTrack.audio_levels[lvl] < 0.0f)
					aTrack.audio_levels[lvl] = 0.0f;
				if (aTrack.audio_levels[lvl] > 2.0f)
					aTrack.audio_levels[lvl] = 2.0f;
			}

			//clips
			if (!v.HasMember("clips") || !v["clips"].IsArray())
				ERROR_EXIT("track::\"clips\" is not an array");
			const rapidjson::Value &clips = v["clips"];
			rapidjson::SizeType num_clips = clips.GetArray().Size();
			for (rapidjson::SizeType c=0; c < num_clips; c++)
			{
				if (!clips.GetArray()[c].IsUint())
					ERROR_EXIT("Invalid tracks::clips::reference");
				unsigned int clip_id = clips.GetArray()[c].GetUint();
				bool valid_clip_id = false;
				for (auto &i : inClips)
				{
					if (clip_id == i.id)
					{
						valid_clip_id = true;
						break;
					}
				}
				if (!valid_clip_id)
					ERROR_EXIT("Invalid tracks::clips::clip_id");
				aTrack.clips.push_back(clip_id);
			}

			//	effects
			if (!v.HasMember("effects") || !v["effects"].IsArray())
				ERROR_EXIT("track::\"effects\" is not an array");
			const rapidjson::Value &effects = v["effects"];
			rapidjson::SizeType num_effects = effects.GetArray().Size();
			for (rapidjson::SizeType e=0; e < num_effects; e++)
			{
				if (!effects.GetArray()[e].IsUint())
					ERROR_EXIT("Invalid tracks::effects::reference");
				unsigned int effect_id = effects.GetArray()[e].GetUint();
				bool valid_effect_id = false;
				for (auto &i : inEffects)
				{
					if (effect_id == i.id)
					{
						valid_effect_id = true;
						break;
					}
				}
				if (!valid_effect_id)
					ERROR_EXIT("Invalid tracks::effects::effect_id");
				aTrack.effects.push_back(effect_id);
			}

			//	notes
			if (!v.HasMember("notes") || !v["notes"].IsArray())
				ERROR_EXIT("track::\"notes\" is not an array");
			const rapidjson::Value &notes = v["notes"];
			rapidjson::SizeType num_notes = notes.GetArray().Size();
			for (rapidjson::SizeType n=0; n < num_notes; n++)
			{
				if (!notes.GetArray()[n].IsUint())
					ERROR_EXIT("Invalid tracks::notes::reference");
				unsigned int note_id = notes.GetArray()[n].GetUint();
				bool valid_note_id = false;
				for (auto &i : inNotes)
				{
					if (note_id == i.id)
					{
						valid_note_id = true;
						break;
					}
				}
				if (!valid_note_id)
					ERROR_EXIT("Invalid tracks::notes::note_id");
				aTrack.notes.push_back(note_id);
			}

			inTracks.push_back(aTrack);
		}
	}
	
	//	"session"
	if (document.HasMember("session"))
	{
		const rapidjson::Value &session = document["session"];
		if (!session.HasMember("horizontal_scroll") || !session["horizontal_scroll"].IsFloat())
			ERROR_EXIT("Missing attribute session::horizontal_scroll");
		inSession.horizontal_scroll = session["horizontal_scroll"].GetFloat();
		if (!session.HasMember("vertical_scroll") || !session["vertical_scroll"].IsFloat())
			ERROR_EXIT("Missing attribute session::vertical_scroll");
		inSession.vertical_scroll = session["vertical_scroll"].GetFloat();
		if (!session.HasMember("zoom_index") || !session["zoom_index"].IsUint())
			ERROR_EXIT("Missing attribute session::zoom_index");
		inSession.zoom_index = session["zoom_index"].GetUint();
		if (!session.HasMember("current_frame") || !session["current_frame"].IsInt64())
			ERROR_EXIT("Missing attribute session::current_frame");
		inSession.current_frame = session["current_frame"].GetInt64();
		if (!session.HasMember("marker_a") || !session["marker_a"].IsInt64())
			ERROR_EXIT("Missing attribute session::marker_a");
		inSession.marker_a = session["marker_a"].GetInt64();
		if (!session.HasMember("marker_b") || !session["marker_b"].IsInt64())
			ERROR_EXIT("Missing attribute session::marker_b");
		inSession.marker_b = session["marker_b"].GetInt64();
	}
	
	//	TODO check if source exists
	//	TODO check if clip end valid

	//	Reset project
	if (clear_media)
	{
		MedoWindow::GetInstance()->RemoveAllMediaSources();
	}
	while (gProject->mTimelineTracks.size() > 0)
		gProject->RemoveTimelineTrack(gProject->mTimelineTracks[0]);

	bool same_resolution = (gProject->mResolution.width == inResolution.width) && (gProject->mResolution.height == inResolution.height);
	gProject->mResolution.width = inResolution.width;
	gProject->mResolution.height = inResolution.height;
	gProject->mResolution.frame_rate = inResolution.frame_rate;

	if (!same_resolution)
	{
		//	Recreate RenderView (different thread)
		sem_id sem = create_sem(0, "invalidate_project");
		if (sem < B_OK)
		{
			printf("Project::LoadProject() Cannot create sem\n");
			exit(1);
		}
		gRenderActor->Async(&RenderActor::AsyncInvalidateProjectSettings, gRenderActor, sem);
		acquire_sem(sem);
		delete_sem(sem);
	}
	
	//	Sources
	for (auto &s : inSources)
	{
		MedoWindow::GetInstance()->AddMediaSource(s.filename.c_str());
	}

	//	Create tracks
	for (auto &i : inTracks)
	{
		TimelineTrack *track = new TimelineTrack;
		track->mVideoEnabled = i.video_enabled;
		track->mAudioEnabled = i.audio_enabled;
		track->mAudioLevels[0] = i.audio_levels[0];
		track->mAudioLevels[1] = i.audio_levels[1];
		gProject->AddTimelineTrack(track);
	}
	
	//	Tracks
	for (auto &t :inTracks)
	{
		for (auto &c : t.clips)
		{
			const CLIP &clip = inClips[c];	//	TODO index
			MediaClip media_clip;
			media_clip.mMediaSource = mMediaSources[clip.source];		//	TODO index
			media_clip.mMediaSourceType = inSources[clip.source].type;
			media_clip.mSourceFrameStart = clip.start;
			media_clip.mSourceFrameEnd = clip.end;
			media_clip.mTimelineFrameStart = clip.timeline;
			media_clip.mTag = clip.tag;
			media_clip.mVideoEnabled = clip.video_enabled;
			media_clip.mAudioEnabled = clip.audio_enabled;
			mTimelineTracks[t.id]->AddClip(media_clip);	//	TODO index
		}
		mTimelineTracks[t.id]->SortClips();
		
		int32 highest_priority = 0;
		for (auto &e : t.effects)
		{
			const EFFECT &effect = inEffects[e];	//	TODO index
			if (effect.media_effect->mPriority > highest_priority)
				highest_priority = effect.media_effect->mPriority;
			mTimelineTracks[t.id]->mEffects.push_back(effect.media_effect);	//	TODO index
		}
		mTimelineTracks[t.id]->mNumberEffectLayers = (t.effects.empty() ? 0 : highest_priority + 1);
		mTimelineTracks[t.id]->SortEffects();

		for (auto &n : t.notes)
		{
			const NOTE &note = inNotes[n];	//	TODO index
			MediaNote media_note;
			media_note.mTimelineFrame = note.timeline;
			media_note.mText = note.text;
			mTimelineTracks[t.id]->mNotes.push_back(media_note);
		}
	}
	
	//	Session
	MedoWindow::GetInstance()->fTimelineView->SetSession(inSession);
	
	return true;
}

/*	FUNCTION:		SaveProject
	ARGUMENTS:		filename
	RETURN:			true if successful
	DESCRIPTION:	Save json file
*/
const bool Project :: SaveProject(FILE *file)
{
	//	Prepare output
	std::vector<SOURCE>	outSources;
	std::vector<CLIP>	outClips;
	std::vector<EFFECT>	outEffects;
	std::vector<NOTE>	outNotes;
	std::vector<TRACK>	outTracks;
	
	//	vSources
	uint32_t source_id = 0;
	for (auto &s : mMediaSources)
	{
		SOURCE aSource;
		aSource.id = source_id++;
		aSource.type = s->GetMediaType();
		aSource.filename =  s->GetFilename();
		outSources.push_back(aSource);
	}
	
	//	vClips
	uint32_t clip_id = 0;
	for (auto &t : mTimelineTracks)
	{
		for (auto &c : t->mClips)
		{
			CLIP aClip;
			source_id = 0;
			for (auto &s : mMediaSources)
			{
				if (c.mMediaSource == s)
					break;
				source_id++;	
			}
			aClip.id = clip_id++;
			aClip.source = source_id;
			aClip.start = c.mSourceFrameStart;
			aClip.end = c.mSourceFrameEnd;
			aClip.timeline = c.mTimelineFrameStart;
			aClip.tag = c.mTag;
			aClip.video_enabled = c.mVideoEnabled;
			aClip.audio_enabled = c.mAudioEnabled;
			outClips.push_back(aClip);
		}
	}
	
	//	vEffects
	uint32_t effect_id = 0;
	for (auto &t : mTimelineTracks)
	{
		for (auto e : t->mEffects)
		{
			EFFECT anEffect;
			anEffect.id = effect_id++;
			anEffect.media_effect = e;
			outEffects.push_back(anEffect);	
		}	
	}

	//	vNotes
	uint32_t note_id = 0;
	for (auto &t : mTimelineTracks)
	{
		for (auto n : t->mNotes)
		{
			NOTE aNote;
			aNote.id = note_id++;
			aNote.timeline = n.mTimelineFrame;
			aNote.text = n.mText.ReplaceAll("\n", "\\n");
			outNotes.push_back(aNote);
		}
	}
	
	//	vTracks
	uint32_t track_id = 0;
	clip_id = 0;
	effect_id = 0;
	note_id = 0;
	for (auto &t : mTimelineTracks)
	{
		TRACK aTrack;
		aTrack.id = track_id++;
		aTrack.video_enabled = t->mVideoEnabled;
		aTrack.audio_enabled = t->mAudioEnabled;
		aTrack.audio_levels[0] = t->mAudioLevels[0];
		aTrack.audio_levels[1] = t->mAudioLevels[1];
		aTrack.global = false;
		for (auto &c : t->mClips)
		{
			aTrack.clips.push_back(clip_id++);	
		}
		for (auto e : t->mEffects)
		{
			aTrack.effects.push_back(effect_id++);
		}
		for (auto n : t->mNotes)
		{
			aTrack.notes.push_back(note_id++);
		}
		outTracks.push_back(aTrack);
	}
	
	TimelineView::SESSION session = MedoWindow::GetInstance()->fTimelineView->GetSession();
	
	//********************	Save project	*********************
	
	char buffer[0x200];	//	512 bytes
	sprintf(buffer, "{\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	//	"medo"
	sprintf(buffer, "\t\"medo\": {\n");
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"version\": %d\n", 1);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t},\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	//	"resolution"
	sprintf(buffer, "\t\"resolution\": {\n");
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"width\": %u,\n", mResolution.width);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"height\": %u,\n", mResolution.height);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"frame_rate\": %f\n", mResolution.frame_rate);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t},\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	//	"sources"
	sprintf(buffer, "\t\"sources\": [\n");
	fwrite(buffer, strlen(buffer), 1, file);
	size_t sources_count = 0;
	for (auto &s : outSources)
	{
		sprintf(buffer, "\t\t{\n");
		fwrite(buffer, strlen(buffer), 1, file);
		
		sprintf(buffer, "\t\t\t\"id\": %u,\n", s.id);
		fwrite(buffer, strlen(buffer), 1, file);
		
		sprintf(buffer, "\t\t\t\"type\": \"%s\",\n", kMediaType[s.type]);
		fwrite(buffer, strlen(buffer), 1, file);
		
		sprintf(buffer, "\t\t\t\"file\": \"%s\"\n", s.filename.c_str());
		fwrite(buffer, strlen(buffer), 1, file);
		
		if (++sources_count < outSources.size())
			sprintf(buffer, "\t\t},\n");
		else
			sprintf(buffer, "\t\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);	
	}
	sprintf(buffer, "\t],\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	//	"clips"
	sprintf(buffer, "\t\"clips\": [\n");
	fwrite(buffer, strlen(buffer), 1, file);
	size_t clips_count = 0;
	for (auto &c : outClips)
	{
		sprintf(buffer, "\t\t{\n");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"id\": %u,\n", c.id);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"source\": %u,\n", c.source);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"start\": %ld,\n", c.start);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"end\": %ld,\n", c.end);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"timeline\": %ld,\n", c.timeline);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"video_enabled\": %s,\n", c.video_enabled ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"audio_enabled\": %s,\n", c.audio_enabled ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"tag\": \"%s\"\n", c.tag.String());
		fwrite(buffer, strlen(buffer), 1, file);
		if (++clips_count < outClips.size())
			sprintf(buffer, "\t\t},\n");
		else
			sprintf(buffer, "\t\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	sprintf(buffer, "\t],\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	//	"effects"
	sprintf(buffer, "\t\"effects\": [\n");
	fwrite(buffer, strlen(buffer), 1, file);
	size_t effects_count = 0;
	for (auto &i : outEffects)
	{
		sprintf(buffer, "\t\t{\n");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"id\": %u,\n", i.id);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"vendor\": \"%s\",\n", i.media_effect->mEffectNode->GetVendorName());
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"name\": \"%s\",\n", i.media_effect->mEffectNode->GetEffectName());
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"start\": %ld,\n", i.media_effect->mTimelineFrameStart);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"end\": %ld,\n", i.media_effect->mTimelineFrameEnd);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"priority\": %u,\n", i.media_effect->mPriority);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"enabled\": %s%s\n", (i.media_effect->mEnabled ? "true" : "false"), i.media_effect->mEffectData ? "," : "");
		fwrite(buffer, strlen(buffer), 1, file);
		if (i.media_effect->mEffectData)
		{
			sprintf(buffer, "\t\t\t\"parameters\":{\n");
			fwrite(buffer, strlen(buffer), 1, file);
			i.media_effect->mEffectNode->SaveParameters(file, i.media_effect);
			sprintf(buffer, "\t\t\t}\n");
			fwrite(buffer, strlen(buffer), 1, file);
		}
		if (++effects_count < outEffects.size())
			sprintf(buffer, "\t\t},\n");
		else
			sprintf(buffer, "\t\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	sprintf(buffer, "\t],\n");
	fwrite(buffer, strlen(buffer), 1, file);

	//	"notes"
	sprintf(buffer, "\t\"notes\": [\n");
	fwrite(buffer, strlen(buffer), 1, file);
	size_t note_count = 0;
	for (auto &n : outNotes)
	{
		sprintf(buffer, "\t\t{\n");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"id\": %u,\n", n.id);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"timeline\": %ld,\n", n.timeline);
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"text\": \"%s\"\n", n.text.String());
		fwrite(buffer, strlen(buffer), 1, file);
		if (++note_count < outNotes.size())
			sprintf(buffer, "\t\t},\n");
		else
			sprintf(buffer, "\t\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	sprintf(buffer, "\t],\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	//	"tracks"
	sprintf(buffer, "\t\"tracks\": [\n");
	fwrite(buffer, strlen(buffer), 1, file);
	size_t tracks_count = 0;
	for (auto &i : outTracks)
	{
		sprintf(buffer, "\t\t{\n");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"id\": %u,\n", i.id);
		fwrite(buffer, strlen(buffer), 1, file);
		if (i.global)
		{
			sprintf(buffer, "\t\t\t\"global\": true,\n");
			fwrite(buffer, strlen(buffer), 1, file);
		}
		sprintf(buffer, "\t\t\t\"video_enabled\": %s,\n", i.video_enabled ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);
		sprintf(buffer, "\t\t\t\"audio_enabled\": %s,\n", i.audio_enabled ? "true" : "false");
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\"levels\": [%f, %f],\n", i.audio_levels[0], i.audio_levels[1]);
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\"clips\": [");
		fwrite(buffer, strlen(buffer), 1, file);
		for (size_t c=0; c < i.clips.size();c++)
		{
			sprintf(buffer, "%u%s", i.clips[c], c+1<i.clips.size() ? ", " : "");
			fwrite(buffer, strlen(buffer), 1, file);
		}
		sprintf(buffer, "],\n");
		fwrite(buffer, strlen(buffer), 1, file);
		
		sprintf(buffer, "\t\t\t\"effects\": [");
		fwrite(buffer, strlen(buffer), 1, file);
		for (size_t e=0; e < i.effects.size();e++)
		{
			sprintf(buffer, "%u%s", i.effects[e], e+1<i.effects.size() ? ", " : "");
			fwrite(buffer, strlen(buffer), 1, file);
		}
		sprintf(buffer, "],\n");
		fwrite(buffer, strlen(buffer), 1, file);

		sprintf(buffer, "\t\t\t\"notes\": [");
		fwrite(buffer, strlen(buffer), 1, file);
		for (size_t n=0; n < i.notes.size(); n++)
		{
			sprintf(buffer, "%u%s", i.notes[n], n+1<i.notes.size() ? ", " : "");
			fwrite(buffer, strlen(buffer), 1, file);
		}
		sprintf(buffer, "]\n");
		fwrite(buffer, strlen(buffer), 1, file);
			
		if (++tracks_count < outTracks.size())
			sprintf(buffer, "\t\t},\n");
		else
			sprintf(buffer, "\t\t}\n");
		fwrite(buffer, strlen(buffer), 1, file);
	}
	sprintf(buffer, "\t],\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	//	"session"
	sprintf(buffer, "\t\"session\": {\n");
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"horizontal_scroll\": %f,\n", session.horizontal_scroll);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"vertical_scroll\": %f,\n", session.vertical_scroll);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"zoom_index\": %d,\n", session.zoom_index);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"current_frame\": %ld,\n", session.current_frame);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"marker_a\": %ld,\n", session.marker_a);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t\t\"marker_b\": %ld\n", session.marker_b);
	fwrite(buffer, strlen(buffer), 1, file);
	sprintf(buffer, "\t}\n}\n");
	fwrite(buffer, strlen(buffer), 1, file);
	
	return true;
}


