/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Path view Gui
 */

#include <interface/View.h>
#include <vector>

class PathView : public BView
{
public:
		PathView(BRect aRect);
		~PathView()						override;

	void	SetObserver(BLooper *looper, BHandler *handler, BMessage *message);
	void	Draw(BRect frame)			override;

	void	MouseDown(BPoint point)		override;
	void	MouseUp(BPoint where)		override;
	void	MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)	override;
	void	MessageReceived(BMessage *msg)		override;

	void	ShowPath(bool show);
	void	ShowFill(bool show);
	void	FillBitmap(BBitmap *bitmap, const std::vector<BPoint> &path);
	void	AllowSizeChange(bool enable) {fAllowSizeChange = enable;}

	void	GetPath(std::vector<BPoint> &points);
	void	SetPath(const std::vector<BPoint> &points);

private:
	std::vector<BPoint>		fPoints;
	std::vector<BPoint>		fPointsMoveAll;
	bool					fMouseTracking;
	int						fMouseTrackingIndex;
	BPoint					fMouseDownPoint;
	bigtime_t				fMouseDownTime;

	bool					fAllowSizeChange;
	bool					fShowPath;
	bool					fShowFill;

	enum MODE {MODE_EDIT_POINT, MODE_ADD_POINTS, MODE_MOVE_POINTS};
	mode_t					fMode;

	BLooper					*fTargetLooper;
	BHandler				*fTargetHandler;
	BMessage				*fTargetMessage;

	void				ContextMenu(BPoint point);
};


