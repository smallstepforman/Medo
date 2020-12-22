/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016
 *	DESCRIPTION:	Medo application
 */
 
#ifndef _MEDO_APPLICATION_H_
#define _MEDO_APPLICATION_H_

#include <app/Application.h>

class MedoWindow;

//====================
class MedoApplication : public BApplication
{
	MedoWindow		*fWindow;
public:
			MedoApplication(int argc, char **argv);
			~MedoApplication();
			
	void	RefsReceived(BMessage *message);
};

#endif	//#ifndef _MEDO_APPLICATION_H_
