/*	PROJECT:		Medo
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		2019, ZenYes Pty Ltd
	DESCRIPTION:	Progress bar
*/

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class ProgressBar : public BView
{
public:
			ProgressBar(BRect frame, const char *name, uint32 resizingMode = B_FOLLOW_LEFT_TOP, uint32 flags = B_WILL_DRAW);
			~ProgressBar() override;

	void	Draw(BRect frame) override;
	void	SetValue(float percentage);

private:
	float	fPercentage;
};

#endif //#ifndef PROGRESS_BAR_H
