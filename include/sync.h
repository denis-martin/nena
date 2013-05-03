/** @file
 * sync.h
 *
 * @brief Generic interface for synchronization primitives
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 * This file uses abstract concepts described in more detail in the boost documentation.
 *
 *  Created on: Dec 12, 2009
 *      Author: Benjamin Behringer
 */

#ifndef SYNC_H_
#define SYNC_H_

#include "messages.h"
#include "mutexes.h"
#include "locks.h"

/*
 * @brief Interface for a factory class that creates the synch primitives.
 *
 * Every systemWrapper should implement this.
 */
class ISyncFactory
{
	public:
	virtual ~ISyncFactory () {}
	
	virtual IMutex * mutexFactory () = 0;
//	virtual ITimedMutex * timedMutexFactory () = 0;
	virtual ISharedMutex * sharedMutexFactory () = 0;
};

#endif
