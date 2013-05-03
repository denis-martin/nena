/** @file
 * locks.h
 *
 * @brief Several Lock Types
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 * This file uses abstract concepts described in more detail in the boost documentation.
 *
 *  Created on: May 24, 2010
 *      Author: Benjamin Behringer
 */

#ifndef LOCKS_H_
#define LOCKS_H_

/// just header file, is platform independent
#include <boost/noncopyable.hpp>

#include <exception>
#include <string>

/**
 * @brief simple lock, acquires ownership of the mutex on construct and releases it on destruction
 */

template<typename Mutex>
class LockGuard : public boost::noncopyable
{
	private:
	Mutex& m;

	public:
	explicit LockGuard(Mutex& m_):
		m(m_)
	{
		m.lock();
	}

	~LockGuard()
	{
		m.unlock();
	}
};

struct defer_lock_t
{};
struct try_to_lock_t
{};
struct adopt_lock_t
{};

/**
 * @brief advanced lock, acquires ownership of the mutex on demand
 */

template<typename Mutex>
class UniqueLock : public boost::noncopyable
{
	public:

	/**
	 * @brief	Bad Flag Value
	 */
	class EBadFlag : public std::exception
	{
	protected:
		std::string msg;

	public:
		EBadFlag() throw () : msg(std::string()) {};
		EBadFlag(const std::string& msg) throw () : msg(msg){};
		virtual ~EBadFlag() throw () {};

		virtual const char* what() const throw () { return msg.c_str(); };
	};

	/**
	 * @brief	Class not owner of mutex
	 */
	class ENoMutex : public std::exception
	{
	protected:
		std::string msg;

	public:
		ENoMutex() throw () : msg(std::string()) {};
		ENoMutex(const std::string& msg) throw () : msg(msg){};
		virtual ~ENoMutex() throw () {};

		virtual const char* what() const throw () { return msg.c_str(); };
	};

	protected:
	Mutex * m;
	bool is_locked;

	public:
	/// creates lock without associated mutex
	UniqueLock ():
		m(NULL),
		is_locked(false)
	{}

	/// creates lock with mutex m_ and locks mutex
	explicit UniqueLock(Mutex& m_):
		m(&m_),
		is_locked(false)
	{
		lock ();
	}

	/// creates lock and adopt lock state
	UniqueLock(Mutex& m_, adopt_lock_t):
		m(&m_),
		is_locked(true)
	{}

	/// creates lock and wait for adopt lock state
	UniqueLock(Mutex& m_, defer_lock_t):
		m(&m_),
		is_locked(false)
	{}

	/// creates lock and tries to adopt lock state
	UniqueLock(Mutex& m_, try_to_lock_t):
		m(&m_),
		is_locked(false)
	{
		try_lock ();
	}

//	/// creates lock and tries to set up timed mutex
//	UniqueLock(Mutex& m_, const AbsTime & time):
//		m(&m_),
//		is_locked(false)
//	{
//		timed_lock (time);
//	}

	~UniqueLock()
	{
		unlock ();
	}

	bool ownsLock () const { return is_locked; }

	Mutex * mutex () const { return * m; }

	Mutex * release ()
	{
		is_locked = false;
		Mutex * ret = m;
		m = NULL;
		return * ret;
	}

	void lock ()
	{
		if (!is_locked)
		{
			m->lock ();
			is_locked = true;
		}
		else
			throw ENoMutex ();
	}

	bool try_lock ()
	{
		if (!is_locked)
			return (is_locked = m->try_lock ());
		else
			throw ENoMutex ();
	}

	void unlock ()
	{
		if (is_locked)
		{
			m->unlock ();
			is_locked = false;
		}
		else
			throw ENoMutex ();
	}

//	bool timed_lock(const AbsTime & abs_time)
//	{
//		if (is_locked)
//			return (is_locked = m->timed_lock (abs_time));
//		else
//			throw ENoMutex ();
//	}
//
//	bool timed_lock(const RelTime & rel_time)
//	{
//		if (is_locked)
//			return (is_locked = m->timed_lock (rel_time));
//		else
//			throw ENoMutex ();
//	}

	void swap (UniqueLock & other)
	{
		std::swap (m, other.m);
		std::swap (is_locked, other.is_locked);
	}
};

/**
 * @brief advanced lock, acquires shared ownership of the mutex on demand
 */

template<typename Mutex>
class SharedLock : public UniqueLock<Mutex>
{
	public:
	/// creates lock without associated mutex
	SharedLock (): UniqueLock<Mutex> ()
	{}

	/// creates lock with mutex m_ and locks mutex
	explicit SharedLock(Mutex& m_): UniqueLock<Mutex> (m_)
	{}

	/// creates lock and adopt lock state
	SharedLock(Mutex& m_, adopt_lock_t t): UniqueLock<Mutex> (m_, t)
	{}

	/// creates lock and wait for adopt lock state
	SharedLock(Mutex& m_, defer_lock_t t): UniqueLock<Mutex> (m_, t)
	{}

	/// creates lock and tries to adopt lock state
	SharedLock(Mutex& m_, try_to_lock_t t): UniqueLock<Mutex> (m_, t)
	{}

//	/// creates lock and tries to set up timed mutex
//	SharedLock(Mutex& m_, const AbsTime & time): UniqueLock<Mutex> (m_, time)
//	{}

	bool try_lock ()
	{
		if (!this->is_locked)
			return (this->is_locked = this->m->try_lock_shared ());
		else
			throw UniqueLock<Mutex>::ENoMutex ();
	}

	void unlock ()
	{
		if (this->is_locked)
		{
			this->m->unlock_shared ();
			this->is_locked = false;
		}
		else
			throw UniqueLock<Mutex>::EENoMutex ();
	}

//	bool timed_lock(const AbsTime & abs_time)
//	{
//		if (this->is_locked)
//			return (this->is_locked = this->m->timed_lock_shared (abs_time));
//		else
//			throw UniqueLock<Mutex>::EENoMutex ();
//	}
//
//	bool timed_lock(const RelTime & rel_time)
//	{
//		if (this->is_locked)
//			return (this->is_locked = this->m->timed_lock_shared (rel_time));
//		else
//			throw UniqueLock<Mutex>::EENoMutex ();
//	}
};


#endif
