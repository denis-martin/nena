/** @file
 * syncBoost.h
 *
 * @brief Sync wrapper for Boost supported systems (e.g. Linux, Windows)
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: May 22, 2010
 *      Author: Benjamin Behringer
 */

#ifndef SYNCBOOST_H_
#define SYNCBOOST_H_

#include "sync.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>


/*
 * @brief Synchronization Primitive "Mutex", based on boost
 */
class CBoostMutex : public IMutex
{
	private:
	boost::mutex m;

	public:
	CBoostMutex () {}
	virtual ~CBoostMutex () {}

	/**
	 * these functions model the Lockable Concept
	 */
	virtual void lock ();
	virtual void unlock ();
	virtual bool try_lock ();
};

//class CBoostTimedMutex : public ITimedMutex
//{
//	private:
//	timed_mutex m;
//
//	public:
//	CBoostTimedMutex () {}
//	virtual ~CBoostTimedMutex () {}
//
//	/**
//	 * these functions model the Lockable Concept
//	 */
//	virtual void lock ();
//	virtual void unlock ();
//	virtual bool try_lock ();
//
//	/**
//	 * these functions extend the Lockable Concept to the timed Lockable Concept
//	 */
//	virtual bool timed_lock(const AbsTime & abs_time);
//	virtual bool timed_lock(const RelTime & rel_time);
//};

class CBoostSharedMutex : public ISharedMutex
{
	private:
	boost::shared_mutex m;

	public:
	CBoostSharedMutex () {}
	virtual ~CBoostSharedMutex () {}

	/**
	 * these functions model the Lockable Concept
	 */
	virtual void lock ();
	virtual void unlock ();
	virtual bool try_lock ();

	/**
	 * these functions extend the Lockable Concept to the timed Lockable Concept
	 */
//	virtual bool timed_lock(const AbsTime & abs_time);
//	virtual bool timed_lock(const RelTime & rel_time);

	/**
	 * for the shared lockable concept
	 */
	virtual void lock_shared();
	virtual bool try_lock_shared();
//	virtual bool timed_lock_shared(const AbsTime & abs_time);
	virtual void unlock_shared();
};

/*
 * @brief Sync Factory based on boost
 */
class CBoostSyncFactory : public ISyncFactory
{
	private:

	public:
	CBoostSyncFactory () {}
	virtual ~CBoostSyncFactory () {}

	virtual IMutex * mutexFactory ();
//	virtual ITimedMutex * timedMutexFactory ();
	virtual ISharedMutex * sharedMutexFactory ();
};

#endif
