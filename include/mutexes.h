/** @file
 * mutexes.h
 *
 * @brief Generic interface for mutexes
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 * This file uses abstract concepts described in more detail in the boost documentation.
 *
 *  Created on: May 24, 2010
 *      Author: Benjamin Behringer
 */

#ifndef MUTEXES_H_
#define MUTEXES_H_

/// just header file, is platform independent
#include <boost/noncopyable.hpp>

/**
 * @brief Interface for a simple mutex.
 *
 * Only one owner is allowed.
 */
class IMutex : boost::noncopyable
{
	public:
	virtual ~IMutex () {}

	/**
	 * these functions model the Lockable Concept
	 */
	virtual void lock () = 0;
	virtual void unlock () = 0;
	virtual bool try_lock () = 0;
};

///**
// * @brief Interface for a mutex with timeouts
// */
//
//class ITimedMutex : public IMutex
//{
//	public:
//	virtual ~ITimedMutex () {}
//	/**
//	 * these functions extend the Lockable Concept to the timed Lockable Concept
//	 */
//	virtual bool timed_lock(const AbsTime & abs_time) = 0;
//	virtual bool timed_lock(const RelTime & rel_time) = 0;
//};

/**
 * @brief Interface for a shared mutex
 *
 * Multiple threads can acquire shared ownership on a shared mutex
 */
class ISharedMutex : public IMutex //public ITimedMutex
{
	public:
	virtual ~ISharedMutex () {}

	/**
	 * these functions extend the Timed Lockable Concept to the shared lockable concept
	 */
	virtual void lock_shared() = 0;
	virtual bool try_lock_shared() = 0;
//	virtual bool timed_lock_shared(const AbsTime & abs_time) = 0;
	virtual void unlock_shared() = 0;

};

#endif
