/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2019-2021
 *	DESCRIPTION:	Project Snapshot (undo/redo)
 */

#include <deque>
#include <cassert>
#include <OS.h>

#include "Actor/Actor.h"
#include "Project.h"
#include "MedoWindow.h"
#include "StatusView.h"
#include "Project_Snapshot.h"

#if 1
#define DEBUG(...)	do {printf(__VA_ARGS__);} while (0)
#else
#define DEBUG(fmt, ...) {}
#endif

static const std::size_t	kMaxSnaps = 40;

/*	FUNCTION:		Memento :: Memento
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
Memento :: Memento()
{
	//	Allocate snap buffers
	system_info si;
	get_system_info(&si);
	fSnapBufferSize = (si.free_memory > 2lu*1024lu*1024lu*1024lu) ? 512*1024 /*512Kb*/ : 128*1024 /*128Kb*/;
	DEBUG("[Memento] buffer element size=%ld\n", fSnapBufferSize);
	for (std::size_t i=0; i < kMaxSnaps; i++)
	{
		fAvailableSnapBuffers.emplace_back(new char [fSnapBufferSize]);
	}

	fCanSnapshot = true;
}

/*	FUNCTION:		Memento :: ~Memento
	ARGS:			n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
Memento :: ~Memento()
{
	Reset(true);
	for (auto &i : fAvailableSnapBuffers)
		delete [] i;
}

/*	FUNCTION:		Memento :: Reset
	ARGS:			can_snap
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
void Memento :: Reset(const bool can_snap)
{
	for (auto &i : fSnaps)
		fAvailableSnapBuffers.push_back(i.buffer);
	fSnaps.clear();

	for (auto &i : fRedos)
		fAvailableSnapBuffers.push_back(i.buffer);
	fRedos.clear();

	fCanSnapshot = can_snap;
}

/*	FUNCTION:		Memento :: Snapshot
	ARGS:			clear_redo
	RETURN:			n/a
	DESCRIPTION:	Take snapshot
*/
void Memento :: Snapshot(const bool clear_redo)
{
	fCanSnapshot = false;
	bigtime_t ts = system_time();
	if (clear_redo)
	{
		for (auto &i : fRedos)
			fAvailableSnapBuffers.push_back(i.buffer);
		fRedos.clear();
	}

	if (fSnaps.size() >= kMaxSnaps)
	{
		MemSnap front = fSnaps.front();
		fAvailableSnapBuffers.push_front(front.buffer);
		fSnaps.pop_front();
	}

	assert(fAvailableSnapBuffers.size() > 0);
	MemSnap aSnap;
	aSnap.buffer = fAvailableSnapBuffers.back();
	fAvailableSnapBuffers.pop_back();

	bool success = false;
	FILE *file = fmemopen(aSnap.buffer, fSnapBufferSize, "wb");
	if (file)
	{
		if (gProject->SaveProject(file))
			success = true;
		fclose(file);
		if (success)
			fSnaps.push_back(aSnap);
		DEBUG("Snapshot time=%ld\n", system_time() - ts);
	}

	if (!success)
	{
		printf("Memento::Snapshot() error\n");
		fAvailableSnapBuffers.push_back(aSnap.buffer);
	}

	DEBUG("Snapshot: buffer_size(%ld), fSnaps.size()=%ld, fRedos.size()=%ld, fAvailableSnapBuffers=%ld\n", strlen(aSnap.buffer), fSnaps.size(), fRedos.size(), fAvailableSnapBuffers.size());
	//printf("%s\n", aSnap.buffer);
	fCanSnapshot = true;
}

/*	FUNCTION:		Memento :: Undo
	ARGS:			none
	RETURN:			true if success
	DESCRIPTION:	Undo
*/
bool Memento :: Undo()
{
	if (fSnaps.empty())
	{
		printf("Memento::Undo() called with no snapshots\n");
		return false;
	}

	//	Prepare Redo
	Snapshot(false);
	fRedos.push_back(fSnaps.back());
	fSnaps.pop_back();

	MemSnap aSnap = fSnaps.back();
	fSnaps.pop_back();

	fCanSnapshot = false;
	bool success = gProject->LoadProject(aSnap.buffer, false);
	fCanSnapshot = true;
	fAvailableSnapBuffers.push_back(aSnap.buffer);
	DEBUG("Memento::Undo() fSnaps.size()=%ld, fRedos.size()=%ld, fAvailableSnapBuffers=%ld\n", fSnaps.size(), fRedos.size(), fAvailableSnapBuffers.size());
	return success;
}

/*	FUNCTION:		Memento :: Redo
	ARGS:			none
	RETURN:			true if success
	DESCRIPTION:	Redo
*/
bool Memento :: Redo()
{
	if (fRedos.empty())
	{
		printf("Memento::fRedos() called with no redos\n");
		return false;
	}

	Snapshot(false);	//	undo redo

	MemSnap aSnap = fRedos.back();
	fRedos.pop_back();

	if (fSnaps.size() >= kMaxSnaps)
	{
		MemSnap front = fSnaps.front();
		fAvailableSnapBuffers.push_front(front.buffer);
		fSnaps.pop_front();
	}

	fCanSnapshot = false;
	bool success = gProject->LoadProject(aSnap.buffer, false);
	fCanSnapshot = true;
	fAvailableSnapBuffers.push_back(aSnap.buffer);
	DEBUG("Memento::Redo() fSnaps.size()=%ld, fRedos.size()=%ld, fAvailableSnapBuffers=%ld\n", fSnaps.size(), fRedos.size(), fAvailableSnapBuffers.size());
	return success;
}

/****************************************
	Project access to Memento class
*****************************************/

/*	FUNCTION:		Project :: Snapshot
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Snapshot project
*/
void Project :: Snapshot()
{
	if (fMemento)
	{
		fMemento->Snapshot(true);
		MedoWindow::GetInstance()->SnapshotUpdate(true, false);
		MedoWindow::GetInstance()->fStatusView->SetText(nullptr);
	}
}

/*	FUNCTION:		Project :: ResetSnapshots
	ARGS:			can_snap
	RETURN:			n/a
	DESCRIPTION:	Dont take snapshots (typically during loading)
*/
void Project :: ResetSnapshots(const bool can_snap)
{
	fMemento->Reset(can_snap);
	MedoWindow::GetInstance()->SnapshotUpdate(false, false);
}

/*	FUNCTION:		Project :: Undo
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Undo last modification
*/
void Project :: Undo()
{
	if (fMemento)
	{
		bool success = fMemento->Undo();
		if (success)
		{
			MedoWindow::GetInstance()->LoadProjectSuccess("Undo success");
			MedoWindow::GetInstance()->SnapshotUpdate(fMemento->GetNumberSnapsAvailable() > 0, fMemento->GetNumberRedosAvailable() > 0);
		}
	}
}

/*	FUNCTION:		Project :: Redo
	ARGS:			none
	RETURN:			n/a
	DESCRIPTION:	Redo last undo
*/
void Project :: Redo()
{
	if (fMemento)
	{
		bool success = fMemento->Redo();
		if (success)
		{
			MedoWindow::GetInstance()->LoadProjectSuccess("Redo success");
			MedoWindow::GetInstance()->SnapshotUpdate(fMemento->GetNumberSnapsAvailable() > 0, fMemento->GetNumberRedosAvailable() > 0);
		}
	}
}
