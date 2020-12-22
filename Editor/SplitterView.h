/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016
 *	DESCRIPTION:	Splitter View
 */

#ifndef SPLITTER_VIEW_H_
#define SPLITTER_VIEW_H_

#ifndef _VIEW_H
#include <interface/View.h>
#endif

class SplitterView : public YView
{
public:
			SplitterView(BRect frame);
			~SplitterView() override;
		
	void	AddChild(BView *view) override;
		
private:
	std::vector<BView *>	fChildren;
};

#endif	//#ifndef SPLITTER_VIEW_H_
