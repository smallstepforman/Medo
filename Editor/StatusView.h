/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Status view
 */

#ifndef _STATUS_VIEW_H_
#define _STATUS_VIEW_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class BStringView;

//=======================
class StatusView : public BView
{
public:
					StatusView(BRect frame);
	void			SetText(const char *);
	void			Draw(BRect frame);
					
private:
	BStringView		*fStringView;
};

#endif	//#ifndef _STATUS_VIEW_H_
