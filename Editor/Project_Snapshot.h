/*	PROJECT:		Medo
 *	AUTHORS:		Zenja Solaja, Melbourne Australia
 *	COPYRIGHT:		Zen Yes Pty Ltd, 2016-2021
 *	DESCRIPTION:	Project Snapshot (undo/redo)
 */

#ifndef PROJECT_SNAPSHOT_H
#define PROJECT_SNAPSHOT_H

#include <deque>

class Memento
{
	struct MemSnap
	{
		char			*buffer;
	};
	std::deque<MemSnap>		fSnaps;
	std::deque<MemSnap>		fRedos;
	std::deque<char *>		fAvailableSnapBuffers;
	std::size_t				fSnapBufferSize;
	bool					fCanSnapshot;	//	dont snapshot while undo in progress

public:
	Memento();
	~Memento();

	void		Reset(const bool pause);
	void		Snapshot(const bool clear_redo);
	bool		Undo();
	bool		Redo();

	std::size_t		GetNumberSnapsAvailable() const		{return fSnaps.size();}
	std::size_t		GetNumberRedosAvailable() const		{return fRedos.size();}
};

#endif //#ifndef PROJECT_SNAPSHOT_H
