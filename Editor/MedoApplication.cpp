/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Medo application
 */

#include <cstdio>
#include <cstring>

#include <StorageKit.h>
#include <Mime.h>

#include "MedoApplication.h"
#include "MedoWindow.h"

static const int32	kMaxRefsReceived = 32;

//======================
MedoApplication :: MedoApplication(int argc, char **argv)
	: BApplication("application/x-vnd.ZenYes.Medo")
{
	fWindow = new MedoWindow;
	fWindow->Show();
	
	if ((argc > 1) && (strstr(argv[1], ".medo")))
	{
		fWindow->LockLooper();
		fWindow->LoadProject(argv[1]);
		fWindow->UnlockLooper();
	}
}

//====================
MedoApplication :: ~MedoApplication()
{
	printf("MedoApplication::~MedoApplication()\n");
	//	fWindow cleaned up by AppServer
	printf("Goodbye\n");	
}

//====================
void MedoApplication :: RefsReceived(BMessage *message)
{		
	int32	count = 0;;
	uint32	type = 0;
	
	message->GetInfo("refs", &type, &count);
	if (count > kMaxRefsReceived)
		count = kMaxRefsReceived;
	
	for (int32 i=0; i < count; i++)
	{
		entry_ref ref;
		if (message->FindRef("refs", i, &ref) == B_NO_ERROR)
		{
			BEntry entry(&ref);
			if (entry.InitCheck() == B_NO_ERROR)
			{
				BPath path;
				entry.GetPath(&path);
				printf("MedoApplication::RefsReceived(%s)\n", path.Path());
				fWindow->LockLooper();
				if (strstr(path.Path(), ".medo"))
					fWindow->LoadProject(path.Path());
				else
					fWindow->AddMediaSource(path.Path());
				fWindow->UnlockLooper();
			}
		}
	}
}
