#include "syncBoost.h"

#include "sync.h"

#include <boost/thread/thread_time.hpp>
#include <boost/thread/mutex.hpp>

using boost::system_time;
using boost::posix_time::from_time_t;

void CBoostMutex::lock ()
{
	m.lock ();
}

void CBoostMutex::unlock ()
{
	m.unlock ();
}

bool CBoostMutex::try_lock ()
{
	return m.try_lock ();
}

/*****************************************************************************/

//void CBoostTimedMutex::lock ()
//{
//	m.lock ();
//}
//
//void CBoostTimedMutex::unlock ()
//{
//	m.unlock ();
//}
//
//bool CBoostTimedMutex::try_lock ()
//{
//	return m.try_lock ();
//}
//
//bool CBoostTimedMutex::timed_lock (const AbsTime & abs_time)
//{
//	system_time t = from_time_t(0);
//	t += boost::posix_time::milliseconds(abs_time.getTime());
//
//	return m.timed_lock (t);
//}
//
//bool CBoostTimedMutex::timed_lock (const RelTime & rel_time)
//{
//	return m.timed_lock (boost::posix_time::milliseconds(rel_time.getTime()));
//}

/*****************************************************************************/

void CBoostSharedMutex::lock ()
{
	m.lock ();
}

void CBoostSharedMutex::unlock ()
{
	m.unlock ();
}

bool CBoostSharedMutex::try_lock ()
{
	return m.try_lock ();
}

//bool CBoostSharedMutex::timed_lock (const AbsTime & abs_time)
//{
//	system_time t = from_time_t(0);
//	t += boost::posix_time::milliseconds(abs_time.getTime());
//
//	return m.timed_lock (t);
//}
//
//bool CBoostSharedMutex::timed_lock (const RelTime & rel_time)
//{
//	return m.timed_lock (boost::posix_time::milliseconds(rel_time.getTime()));
//}

void CBoostSharedMutex::lock_shared()
{
	m.lock_shared();
}

bool CBoostSharedMutex::try_lock_shared()
{
	return m.try_lock_shared();
}

//bool CBoostSharedMutex::timed_lock_shared(const AbsTime & abs_time)
//{
//	system_time t = from_time_t(0);
//	t += boost::posix_time::milliseconds(abs_time.getTime());
//
//	return m.timed_lock_shared(t);
//}

void CBoostSharedMutex::unlock_shared()
{
	m.unlock_shared();
}

/*****************************************************************************/

IMutex * CBoostSyncFactory::mutexFactory ()
{
	return new CBoostMutex ();
}

//ITimedMutex * CBoostSyncFactory::timedMutexFactory ()
//{
//	return new CBoostTimedMutex ();
//}

ISharedMutex * CBoostSyncFactory::sharedMutexFactory ()
{
	return new CBoostSharedMutex ();
}
