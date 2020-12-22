/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Curves view
 */

#include <interface/View.h>

class CurvesView : public BView
{
public:
		CurvesView(BRect aRect, BHandler *parent, BMessage *msg);
		~CurvesView()					override;

	enum CURVE_COLOUR {CURVE_RED, CURVE_GREEN, CURVE_BLUE, NUMBER_CURVES};

	void	AttachedToWindow()			override;
	void	Draw(BRect frame)			override;

	void	MouseDown(BPoint point)		override;
	void	MouseUp(BPoint where)		override;
	void	MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)	override;

	enum	INTERPOLATION {INTERPOLATION_CATMULL_ROM, INTERPOLATION_BEIZER};
	void	SetInterpolation(INTERPOLATION interpolation);

	void	SetActiveColour(CURVE_COLOUR colour);
	void	SetColourValues(CURVE_COLOUR colour, float *values);
	void	Reset();

	float	*GetColour(CURVE_COLOUR colour) {return fColourControls[colour].v;}
	void	SetWhiteBalance(const rgb_color &white, bool send_msg);

private:
	struct COLOUR_COMPONENT
	{
		float		v[4];
		BPoint		points[4];
	};
	COLOUR_COMPONENT	fColourControls[NUMBER_CURVES];
	int					fColourIndex;

	bool				fMouseTracking;
	int					fMouseTrackingIndex;
	INTERPOLATION		fInterpolation;

	BHandler			*fParent;
	BMessage			*fMessage;
};


