/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Audio Mixer
 */

#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>

#include "Gui/DualSlider.h"

#include "AudioMixer.h"
#include "Language.h"
#include "PersistantWindow.h"
#include "Project.h"
#include "Theme.h"

enum kMixerMessages
{
	eMsgSlider	= 'amsl',
};
static const float kDualSliderOffset = 70.0f;
static const float kDualSliderWidth = 120.0f;
/*************************************
	AudioMixerView
**************************************/
class AudioMixerView : public BView
{
	std::vector<DualSlider *>	fSliders;
	std::vector<std::pair<float, float>>	fVisualizations;

public:
	AudioMixerView(BRect bounds)
	: BView(bounds, nullptr, B_FOLLOW_ALL, B_WILL_DRAW | B_SCROLL_VIEW_AWARE | B_FRAME_EVENTS)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		float offset = kDualSliderOffset;
		for (auto &i : gProject->mTimelineTracks)
		{
			DualSlider *slider = new DualSlider(BRect(offset, 16, offset+kDualSliderOffset + 32, 16+400), nullptr, i->mName.String(), new BMessage(eMsgSlider), 0, 200,
												GetText(TXT_EFFECTS_COMMON_L), GetText(TXT_EFFECTS_COMMON_R));
			AddChild(slider);
			slider->SetValue(0, 100*i->mAudioLevels[0]);
			slider->SetValue(1, 100*i->mAudioLevels[1]);
			fSliders.push_back(slider);
			fVisualizations.push_back(std::pair<float, float>(0.0f, 0.f));
			offset += kDualSliderWidth+kDualSliderOffset;
		}
	}

	~AudioMixerView()	override
	{ }

	void AttachedToWindow() override
	{
		for (auto &i : fSliders)
			i->SetTarget(this, Window());
	}

	void FrameResized(float w, float h) override
	{
		for (auto &i : fSliders)
			i->Invalidate(Bounds());
	}
	void ScrollTo(BPoint point) override
	{
		BView::ScrollTo(point);
		for (auto &i : fSliders)
			i->Invalidate(Bounds());
	}

	void Draw(BRect frame) override
	{
		const float kSliderOffsetY = 32 + 16;
		const float kSliderHeight = (400-24) - kSliderOffsetY;
		for (int i=0; i < gProject->mTimelineTracks.size(); i++)
		{
			SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
			MovePenTo(kDualSliderOffset + i*(kDualSliderWidth+kDualSliderOffset), 400+16+32);
			DrawString(gProject->mTimelineTracks[i]->mName);

			//	Visualisation
			SetHighColor(Theme::GetUiColour(UiColour::eListSelection));
			FillRect(BRect(16 + i*(kDualSliderWidth+kDualSliderOffset), kSliderOffsetY, 16 + i*(kDualSliderWidth+kDualSliderOffset) + 16, kSliderOffsetY + kSliderHeight));
			FillRect(BRect(16 + 20 + i*(kDualSliderWidth+kDualSliderOffset), kSliderOffsetY, 16 + 20 + i*(kDualSliderWidth+kDualSliderOffset) + 16, kSliderOffsetY + kSliderHeight));

			SetHighColor(0, 192, 0);
			FillRect(BRect(16 + i*(kDualSliderWidth+kDualSliderOffset), kSliderOffsetY + (kSliderHeight - kSliderHeight*fVisualizations[i].first), 16 + i*(kDualSliderWidth+kDualSliderOffset) + 16, kSliderOffsetY + kSliderHeight));
			FillRect(BRect(16 + 20 + i*(kDualSliderWidth+kDualSliderOffset), kSliderOffsetY + (kSliderHeight - kSliderHeight*fVisualizations[i].second), 16 + 20 + i*(kDualSliderWidth+kDualSliderOffset) + 16, kSliderOffsetY + kSliderHeight));

			//printf("Draw(%d) %f, %f\n", i, fVisualizations[i].first, fVisualizations[i].second);
		}
	}

	void ProjectInvalidated()
	{
		for (auto &i : fSliders)
		{
			RemoveChild(i);
			delete i;
		}
		fSliders.clear();
		fVisualizations.clear();

		float offset = kDualSliderOffset;
		for (auto &i : gProject->mTimelineTracks)
		{
			DualSlider *slider = new DualSlider(BRect(offset, 16, offset+70, 16+400), nullptr, i->mName.String(), new BMessage(eMsgSlider), 0, 200,
												GetText(TXT_EFFECTS_COMMON_L), GetText(TXT_EFFECTS_COMMON_R));
			AddChild(slider);
			slider->SetValue(0, 100*i->mAudioLevels[0]);
			slider->SetValue(1, 100*i->mAudioLevels[1]);
			fSliders.push_back(slider);
			fVisualizations.push_back(std::pair<float, float>(0.0f, 0.f));
			offset += kDualSliderWidth+kDualSliderOffset;
		}
	}

	void SliderUpdate()
	{
		assert(gProject->mTimelineTracks.size() == fSliders.size());
		for (int i=0; i < fSliders.size(); i++)
		{
			gProject->mTimelineTracks[i]->mAudioLevels[0] = fSliders[i]->GetValue(0)/100.0f;
			gProject->mTimelineTracks[i]->mAudioLevels[1] = fSliders[i]->GetValue(1)/100.0f;
		}
	}

	void VisualiseLevels(int32 track_idx, float left, float right)
	{
		//printf("VisualiseLevels(%d, %f, %f)\n", track_idx, left, right);
		if (track_idx < 0)
		{
			for (auto &i : fVisualizations)
			{
				i.first = 0.0f;
				i.second = 0.0f;
			}
			return;
		}
		else if (track_idx >= fVisualizations.size())
		{
			Invalidate();
			return;
		}
		else
		{
			fVisualizations[track_idx].first = left;
			fVisualizations[track_idx].second = right;
		}
	}
};

/*************************************
	AudioMixer
**************************************/

/*	FUNCTION:		AudioMixer :: AudioMixer
	ARGS:			frame
					title
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
AudioMixer :: AudioMixer(BRect frame, const char *title)
	: PersistantWindow(frame, title)
{
	BRect scroll_view = Bounds();
	scroll_view.right -= B_V_SCROLL_BAR_WIDTH + 2;
	scroll_view.bottom -= B_H_SCROLL_BAR_HEIGHT + 2;
	fMixerView = new AudioMixerView(scroll_view);
	fScrollView = new BScrollView("list_scroll", fMixerView, B_FOLLOW_ALL, 0, true, true);
	fScrollView->ScrollBar(B_HORIZONTAL)->SetRange(0.0f, 100.0f);
	fScrollView->ScrollBar(B_HORIZONTAL)->SetProportion(1.0f);
	fScrollView->ScrollBar(B_HORIZONTAL)->SetValue(0.0f);
	fScrollView->ScrollBar(B_VERTICAL)->SetRange(0.0f, 100.0f);
	fScrollView->ScrollBar(B_VERTICAL)->SetProportion(1.0f);
	fScrollView->ScrollBar(B_VERTICAL)->SetValue(0.0f);
	AddChild(fScrollView);

	mMsgVisualiseLevels = new BMessage(kMsgVisualiseLevels);
	mMsgVisualiseLevels->AddInt32("track", 0);
	mMsgVisualiseLevels->AddFloat("left", 0.0f);
	mMsgVisualiseLevels->AddFloat("right", 0.0f);
}

/*	FUNCTION:		AudioMixer :: ~AudioMixer
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
AudioMixer :: ~AudioMixer()
{
	delete mMsgVisualiseLevels;
}

/*	FUNCTION:		AudioMixer :: FrameResized
	ARGS:			width
					height
	RETURN:			n/a
	DESCRIPTION:	Adjust scroll bar proportions
*/
void AudioMixer :: FrameResized(float width, float height)
{
	float required_width = kDualSliderOffset + gProject->mTimelineTracks.size()*(kDualSliderWidth+kDualSliderOffset) - B_V_SCROLL_BAR_WIDTH;
	float required_height = 480 - B_H_SCROLL_BAR_HEIGHT;
	fScrollView->ScrollBar(B_HORIZONTAL)->SetProportion(required_width < width ? 1.0f : width/required_width);
	fScrollView->ScrollBar(B_VERTICAL)->SetProportion(required_height < height ? 1.0f : height/required_height);
	fMixerView->Invalidate();
}

/*	FUNCTION:		AudioMixer :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Process window messages
*/
void AudioMixer :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case eMsgSlider:
			fMixerView->SliderUpdate();
			break;

		case kMsgProjectInvalidated:
			fMixerView->ProjectInvalidated();
			FrameResized(Bounds().Width(), Bounds().Height());
			break;

		case kMsgVisualiseLevels:
		{
			int32 track_idx;
			float left, right;
			if ((msg->FindInt32("track", &track_idx) == B_OK) &&
				(msg->FindFloat("left", &left) == B_OK) &&
				(msg->FindFloat("right", &right) == B_OK))
				fMixerView->VisualiseLevels(track_idx, left, right);
			break;
		}

		default:
			BWindow::MessageReceived(msg);
	}
}

