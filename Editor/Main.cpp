/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Medo main
 */

#include <cstdlib> 
#include "MedoApplication.h"
#include "Actor/ActorManager.h"

//=====================
int main(int argc, char **argv)
{
	setenv("MESA_GL_VERSION_OVERRIDE", "3.3", true);
	
	yarra::ActorManager actor_manager(8);	//	TODO send Quit() message

	MedoApplication *app = new MedoApplication(argc, argv);
	app->Run();
	delete app;
	return 0;	
}
