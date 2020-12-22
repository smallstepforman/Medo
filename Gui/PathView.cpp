#include <cstdio>
#include <cassert>

#include <InterfaceKit.h>
#include "PathView.h"

#include "Editor/Language.h"
#include "Editor/MedoWindow.h"
#include "Editor/OutputView.h"
#include "Editor/Project.h"

static const float	kControlPointSize = 8.0f;

enum kContextMessages
{
	kMsgContextAddPoint				= 'pvm0',
	kMsgContextChangeMode,
	kMsgContextInsertPointBefore,
	kMsgContextInsertPointAfter,
	kMsgContextDeletePoint,
	kMsgContextDeleteAllPoints,
	kMsgContextMoveAllPoints,
};

/*	FUNCTION:		PathView :: PathView
	ARGS:			frame
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
PathView :: PathView(BRect frame)
	: BView(frame, "PathView", B_FOLLOW_ALL, B_WILL_DRAW | B_TRANSPARENT_BACKGROUND),
	fTargetLooper(nullptr), fTargetHandler(nullptr), fTargetMessage(nullptr)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	fMouseTracking = false;
	fMode = MODE_EDIT_POINT;
	fMouseDownTime = system_time();
	fShowPath = true;
	fShowFill = false;
	fAllowSizeChange = true;
}

/*	FUNCTION:		PathView :: ~PathView
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
PathView :: ~PathView()
{}

/*	FUNCTION:		PathView :: SetObserver
	ARGS:			looper, handler
					message
	RETURN:			n/a
	DESCRIPTION:	Notify target
*/
void PathView :: SetObserver(BLooper *looper, BHandler *handler, BMessage *message)
{
	assert(looper);
	assert(handler);
	assert(message);

	fTargetLooper = looper;
	fTargetHandler = handler;
	fTargetMessage = message;
}

/*	FUNCTION:		PathView :: ShowPath
	ARGS:			show
	RETURN:			n/a
	DESCRIPTION:	Set state
*/
void PathView :: ShowPath(bool show)
{
	LockLooper();
	fShowPath = show;
	Invalidate();
	UnlockLooper();
}

/*	FUNCTION:		PathView :: ShowFill
	ARGS:			show
	RETURN:			n/a
	DESCRIPTION:	Set state
*/
void PathView :: ShowFill(bool show)
{
	LockLooper();
	fShowFill = show;
	Invalidate();
	UnlockLooper();
}

/*	FUNCTION:		PathView :: MouseDown
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook when mouse down
*/
void PathView :: MouseDown(BPoint point)
{
	//	Cater for resize layout
	OutputView *output_view = MedoWindow::GetInstance()->GetOutputView();
	BRect bounds = output_view->Bounds();
	BPoint scaled_point = MedoWindow::GetInstance()->GetOutputView()->GetProjectConvertedMouseDown(point);

	const float kGrace = 3.0f / bounds.Height();
	int selected_index = 0;
	for (auto i : fPoints)
	{
		if ((scaled_point.x >= i.x - kGrace*kControlPointSize) &&
			(scaled_point.x < i.x + kGrace*kControlPointSize) &&
			(scaled_point.y >= i.y - kGrace*kControlPointSize) &&
			(scaled_point.y < i.y + kGrace*kControlPointSize))
		{
			fMouseTracking = true;
			fMouseTrackingIndex = selected_index;
			break;
		}
		selected_index++;
	}

	uint32 buttons;
	BMessage* msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", (int32*)&buttons);
	fMouseDownPoint = scaled_point;
	if (buttons & B_SECONDARY_MOUSE_BUTTON)
	{
		ContextMenu(point);
		fMouseTracking = false;
		Invalidate();
		return;
	}

	if (fMode == MODE_ADD_POINTS)
	{
		fPoints.push_back(scaled_point);
		fMouseTrackingIndex = fPoints.size() - 1;
		fMouseTracking = false;
		Invalidate();
		return;
	}

	if (fMouseTracking)
	{
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

		bigtime_t ts = system_time();
		if (ts - fMouseDownTime < 200'000)
		{
			fMode = MODE_MOVE_POINTS;
		}
		fMouseDownTime = ts;

		if (fMode == MODE_MOVE_POINTS)
		{
			fPointsMoveAll.clear();
			fPointsMoveAll = fPoints;
		}
	}

	if (!fMouseTracking)
	{
		fMouseTrackingIndex = -1;
		if (fMode == MODE_MOVE_POINTS)
			fMode = MODE_EDIT_POINT;
	}
	Invalidate();

	if (!fMouseTracking)
		Parent()->MouseDown(point);
}

/*	FUNCTION:		PathView :: MouseUp
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook when mouse up
*/
void PathView :: MouseUp(BPoint point)
{
	if (!fMouseTracking)
		Parent()->MouseUp(point);
	fMouseTracking = false;
}

/*	FUNCTION:		PathView :: MouseMoved
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Hook when mouse moved
*/
void PathView :: MouseMoved(BPoint where, uint32 code, const BMessage *drag_message)
{
	if (!fMouseTracking)
	{
		Parent()->MouseMoved(where, code, drag_message);
		return;
	}

	BRect frame = Bounds();
	if (where.x < 0)
		where.x = 0;
	if (where.x > frame.Width())
		where.x = frame.Width();

	if (where.y < 0)
		where.y = 0;
	if (where.y > frame.Height())
		where.y = frame.Height();

	//	Cater for resize layout
	OutputView *output_view = MedoWindow::GetInstance()->GetOutputView();
	BRect bounds = output_view->Bounds();
	float ratio_x = bounds.Width() / gProject->mResolution.width;
	float ratio_y = bounds.Height() / gProject->mResolution.height;
	float r = (ratio_x < ratio_y) ? ratio_x : ratio_y;
	float fZoomFactor = 1.0f;
	BRect aRect;
	aRect.left = 0.5f*(bounds.Width() - (gProject->mResolution.width*fZoomFactor*r));
	aRect.right = aRect.left + gProject->mResolution.width*fZoomFactor*r;
	aRect.top = 0.5f*(bounds.Height() - (gProject->mResolution.height*fZoomFactor*r));
	aRect.bottom = aRect.top + gProject->mResolution.height*fZoomFactor*r;
	BPoint scaled_point((where.x-aRect.left)/aRect.Width(), (where.y-aRect.top)/aRect.Height());


	if (fMode != MODE_MOVE_POINTS)
		fPoints[fMouseTrackingIndex].Set(scaled_point.x, scaled_point.y);
	else
	{
		assert(fPoints.size() == fPointsMoveAll.size());
		for (size_t i=0; i < fPoints.size(); i++)
		{
			fPoints[i].x = fPointsMoveAll[i].x + scaled_point.x - fMouseDownPoint.x;
			fPoints[i].y = fPointsMoveAll[i].y + scaled_point.y - fMouseDownPoint.y;
		}
	}
	Invalidate();
	fTargetLooper->PostMessage(fTargetMessage, fTargetHandler);
}


/*	FUNCTION:		PathView :: ContextMenu
	ARGS:			point
	RETURN:			n/a
	DESCRIPTION:	Show context menu
*/
void PathView :: ContextMenu(BPoint point)
{
	ConvertToScreen(&point);

	//	Initialise PopUp menu
	BPopUpMenu *aPopUpMenu = new BPopUpMenu("ContextMenuClip", false, false);
	aPopUpMenu->SetAsyncAutoDestruct(true);

	BMenuItem *aMenuItem_add_point = new BMenuItem(GetText(TXT_PATH_VIEW_ADD_POINT), new BMessage(kMsgContextAddPoint));
	aPopUpMenu->AddItem(aMenuItem_add_point);

	BMenuItem *aMenuItem_add_points = new BMenuItem(GetText(TXT_PATH_VIEW_ADD_POINTS_MODE), new BMessage(kMsgContextChangeMode));
	aPopUpMenu->AddItem(aMenuItem_add_points);

	BMenuItem *aMenuItem_insert_before = new BMenuItem(GetText(TXT_PATH_VIEW_INSERT_POINT_BEFORE), new BMessage(kMsgContextInsertPointBefore));
	aPopUpMenu->AddItem(aMenuItem_insert_before);

	BMenuItem *aMenuItem_insert_after = new BMenuItem(GetText(TXT_PATH_VIEW_INSERT_POINT_AFTER), new BMessage(kMsgContextInsertPointAfter));
	aPopUpMenu->AddItem(aMenuItem_insert_after);

	BMenuItem *aMenuItem_delete_point = new BMenuItem(GetText(TXT_PATH_VIEW_DELETE_POINT), new BMessage(kMsgContextDeletePoint));
	aPopUpMenu->AddItem(aMenuItem_delete_point);

	BMenuItem *aMenuItem_delete_points = new BMenuItem(GetText(TXT_PATH_VIEW_DELETE_ALL_POINTS), new BMessage(kMsgContextDeleteAllPoints));
	aPopUpMenu->AddItem(aMenuItem_delete_points);

	BMenuItem *aMenuItem_move_all = new BMenuItem(GetText(TXT_PATH_VIEW_MOVE_ALL_POINTS), new BMessage(kMsgContextMoveAllPoints));
	aPopUpMenu->AddItem(aMenuItem_move_all);

	aPopUpMenu->SetTargetForItems(this);
	aPopUpMenu->Go(point, true /*notify*/, false /*stay open when mouse away*/, true /*async*/);

	//	Enable/Disable options based on mode
	switch (fMode)
	{
		case MODE_ADD_POINTS:
			aMenuItem_add_points->SetMarked(true);

			aMenuItem_add_point->SetEnabled(false);
			aMenuItem_insert_before->SetEnabled(false);
			aMenuItem_insert_after->SetEnabled(false);
			aMenuItem_delete_point->SetEnabled(false);
			aMenuItem_delete_points->SetEnabled(false);
			aMenuItem_move_all->SetEnabled(false);
			break;

		case MODE_MOVE_POINTS:
			aMenuItem_move_all->SetMarked(true);

			aMenuItem_add_point->SetEnabled(false);
			aMenuItem_add_points->SetEnabled(false);
			aMenuItem_insert_before->SetEnabled(false);
			aMenuItem_insert_after->SetEnabled(false);
			aMenuItem_delete_point->SetEnabled(false);
			aMenuItem_delete_points->SetEnabled(false);
			break;

		default:
			break;
	}

	if (!fAllowSizeChange)
	{
		aMenuItem_add_point->SetEnabled(false);
		aMenuItem_add_points->SetEnabled(false);
		aMenuItem_insert_before->SetEnabled(false);
		aMenuItem_insert_after->SetEnabled(false);
		aMenuItem_delete_point->SetEnabled(false);
		aMenuItem_delete_points->SetEnabled(false);
	}

	if (fPoints.size() == 0)
	{
		aMenuItem_insert_before->SetEnabled(false);
		aMenuItem_insert_after->SetEnabled(false);
		aMenuItem_delete_point->SetEnabled(false);
		aMenuItem_delete_points->SetEnabled(false);
		aMenuItem_move_all->SetEnabled(false);
	}

	//All BPopUpMenu items are freed when the popup is closed
}

/*	FUNCTION:		PathView :: MessageReceived
	ARGS:			msg
	RETURN:			n/a
	DESCRIPTION:	Context menu messages
*/
void PathView :: MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case kMsgContextAddPoint:
			fPoints.push_back(fMouseDownPoint);
			fMouseTrackingIndex = fPoints.size() - 1;
			break;
		case kMsgContextChangeMode:
			if (fMode == MODE_EDIT_POINT)
				fMode = MODE_ADD_POINTS;
			else
				fMode = MODE_EDIT_POINT;
			break;
		case kMsgContextInsertPointBefore:
			fPoints.insert(fPoints.begin() + fMouseTrackingIndex, fMouseDownPoint);
			break;
		case kMsgContextInsertPointAfter:
			fPoints.insert(fPoints.begin() + fMouseTrackingIndex + 1, fMouseDownPoint);
			fMouseTrackingIndex++;
			break;
		case kMsgContextDeletePoint:
			fPoints.erase(fPoints.begin() + fMouseTrackingIndex);
			if (fMouseTrackingIndex >= fPoints.size())
				fMouseTrackingIndex = fPoints.size() - 1;
			break;
		case kMsgContextDeleteAllPoints:
			fPoints.clear();
			fMouseTrackingIndex = -1;
			break;
		case kMsgContextMoveAllPoints:
			if (fMode != MODE_EDIT_POINT)
				fMode = MODE_EDIT_POINT;
			else
				fMode = MODE_MOVE_POINTS;
			Invalidate();
			break;

		default:
			BView::MessageReceived(msg);
			return;
	}

	fTargetLooper->PostMessage(fTargetMessage, fTargetHandler);
}

/*	FUNCTION:		PathView :: GetPath
	ARGS:			points
	RETURN:			n/a
	DESCRIPTION:	Called from Effect_Mask
*/
void PathView :: GetPath(std::vector<BPoint> &points)
{
	points = fPoints;
}

/*	FUNCTION:		PathView :: SetPath
	ARGS:			points
	RETURN:			n/a
	DESCRIPTION:	Called from Effect_Mask
*/
void PathView :: SetPath(const std::vector<BPoint> &points)
{
	fPoints = points;
}

/*	FUNCTION:		PathView :: Draw
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Draw path in OutputView
*/
void PathView :: Draw(BRect)
{
	//	Cater for resize layout
	OutputView *output_view = MedoWindow::GetInstance()->GetOutputView();

	//	TODO cater for zoom/pan
	float zoom_factor = output_view->GetZoomFactor();
	BPoint zoom_offset = output_view->GetZoomOffset();

	BRect bounds = output_view->Bounds();
	float ratio_x = bounds.Width() / gProject->mResolution.width;
	float ratio_y = bounds.Height() / gProject->mResolution.height;
	float r = (ratio_x < ratio_y) ? ratio_x : ratio_y;
	float fZoomFactor = 1.0f;
	BRect aRect;
	aRect.left = 0.5f*(bounds.Width() - (gProject->mResolution.width*fZoomFactor*r));
	aRect.right = aRect.left + gProject->mResolution.width*fZoomFactor*r;
	aRect.top = 0.5f*(bounds.Height() - (gProject->mResolution.height*fZoomFactor*r));
	aRect.bottom = aRect.top + gProject->mResolution.height*fZoomFactor*r;
	float w = aRect.Width();
	float h = aRect.Height();

#if 0
	printf("*** PathView::Draw() *****\n");
	for (auto &i : fPoints)
		printf("(%f, %f)\n", i.x, i.y);
	printf("w=%f, h=%f   ", w, y);  bounds.PrintToStream();
#endif

	//	Lines
	if (fShowPath)
	{
		SetHighColor({255, 32, 32, 255});
		SetPenSize(4.0f);

		if (fPoints.size() > 1)
		{
			for (size_t i=0; i < fPoints.size() - 1; i++)
			{
				StrokeLine(BPoint(aRect.left + fPoints[i].x*w, aRect.top + fPoints[i].y*h), BPoint(aRect.left + fPoints[i+1].x*w, aRect.top + fPoints[i+1].y*h));
			}
			StrokeLine(BPoint(aRect.left + fPoints[0].x*w, aRect.top + fPoints[0].y*h), BPoint(aRect.left + fPoints[fPoints.size()-1].x*w, aRect.top + fPoints[fPoints.size()-1].y*h));
		}
	}

	//	show fill
	if (fShowFill && (fPoints.size() > 2))
	{
		std::vector<BPoint> scaled_points;
		for (auto &i : fPoints)
			scaled_points.push_back(BPoint(aRect.left + i.x*w, aRect.top + i.y*h));

		BPolygon aPolygon(scaled_points.data(), scaled_points.size());
		SetHighColor(128, 128, 128);
		FillPolygon(&aPolygon);

	}

	//	Control points
	if (fShowPath)
	{
		int point_idx = 0;
		font_height fh;
		be_plain_font->GetHeight(&fh);
		for (auto &i : fPoints)
		{
			if ((point_idx == fMouseTrackingIndex) || (fMode == MODE_MOVE_POINTS))
				SetHighColor({255, 255, 0, 255});
			else
				SetHighColor({255, 32, 32, 255});

			if (point_idx < 9)
				FillRect(BRect(aRect.left + i.x*w - kControlPointSize, aRect.top + i.y*h - kControlPointSize, aRect.left + i.x*w + kControlPointSize, aRect.top + i.y*h + kControlPointSize));
			else
				FillRect(BRect(aRect.left + i.x*w - kControlPointSize, aRect.top + i.y*h - kControlPointSize, aRect.left + i.x*w + 1.5f*kControlPointSize, aRect.top + i.y*h + kControlPointSize));

			if ((point_idx == fMouseTrackingIndex) || (fMode == MODE_MOVE_POINTS))
				SetHighColor({0, 0, 32, 255});
			else
				SetHighColor({255, 255, 255, 255});
#if 1
			MovePenTo(aRect.left + i.x*w - 1.25f*fh.descent, aRect.top + i.y*h + 1.25f*fh.descent);
			char label[8];
			sprintf(label, "%u", point_idx+1);
			DrawString(label);
#endif
			point_idx++;
		}
	}
}

/**************************************
	Fill Bitmap
***************************************/
#include <interface/Bitmap.h>

class PathFillerView : public BView
{
	std::vector<BPoint> fPoints;
public:
	PathFillerView(BBitmap *parent)
		: BView(parent->Bounds(), "pathFillerVIew", B_FOLLOW_NONE, B_WILL_DRAW)
	{ }
	void SetPoints(const std::vector<BPoint> &points) {fPoints = points;}
	void Draw(BRect frame)
	{
		float w = frame.Width();
		float h = frame.Height();

		std::vector<BPoint> scaled_points;
		for (auto &i : fPoints)
			scaled_points.push_back(BPoint(i.x*w, i.y*h));

		BPolygon aPolygon(scaled_points.data(), scaled_points.size());

		SetHighColor(255, 255, 255, 255);
		FillPolygon(&aPolygon);
	}
};
static PathFillerView	*sPathFillerView = nullptr;
void PathView :: FillBitmap(BBitmap *bitmap, const std::vector<BPoint> &path)
{
	if (!sPathFillerView)
	{
		sPathFillerView = new PathFillerView(bitmap);
		bitmap->AddChild(sPathFillerView);
	}
	memset(bitmap->Bits(), 0x00, bitmap->Bounds().Width()*bitmap->Bounds().Height()*4);

	if (fPoints.size() < 3)
		return;
	sPathFillerView->SetPoints(path);
	sPathFillerView->LockLooper();
	sPathFillerView->Draw(bitmap->Bounds());
	sPathFillerView->Sync();
	sPathFillerView->UnlockLooper();
}

