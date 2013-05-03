/**
 * systemBoost.cpp
 *
 * @brief System wrapper for Boost supported systems (e.g. Linux, Windows)
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 10, 2008
 *      Author: denis
 *  Modified on: Aug 29, 2009
 *      Author: Benjamin Behringer
 *      Changes: added netAdaptBoostUDP, scheduler, appInterface
 */

#include "systemBoost.h"
#include "debug.h"
#include "messages.h"

#include "netAdaptBoostUDP.h"
#include "netAdaptBoostTap.h"
#include "netAdaptBoostRaw.h"
#include "boostSchedulerMt.h"
#include "socketAppConnector.h"
#include "memAppConnector.h"
#include "syncBoost.h"

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/system/error_code.hpp>
#include <boost/ptr_container/ptr_list.hpp>

#include <cstdlib>

using namespace std;

using boost::mutex;
using boost::lock_guard;
using boost::shared_mutex;
using boost::ptr_list;
using boost::shared_ptr;
using boost::posix_time::second_clock;
using boost::posix_time::ptime;

using std::ios;
using std::endl;
using std::list;
using std::string;
using std::exception;

// Pointer to Architecture specific channel, filled by nodearch
IDebugChannel * globalChannel;

const string systemId = "system://boost";

/**
 * @brief Log message to file
 */
void CDebugChannel::log(std::string msg)
{
	lock_guard<mutex> dbglock(m);

	if (logfile.is_open()) {
		logfile << boost::format("%1$09.3f %2%") % second_clock::local_time() % msg << std::endl;
	}
}
;

/**
 * @brief Log formatted message to file
 */
void CDebugChannel::log(boost::format fmt)
{
	lock_guard<mutex> dbglock(m);

	if (logfile.is_open()) {
		logfile << boost::format("%1$09.3f %2%") % second_clock::local_time() % fmt << std::endl;
	}
}
;

/**
 * @brief Print a message.
 */
void CDebugChannel::print(std::string msg)
{
	lock_guard<mutex> dbglock(m);

	std::cerr << boost::format("%1$09.3f %2%") % second_clock::local_time() % msg << std::endl;
}
;

/**
 * @brief Print a formatted message.
 */
void CDebugChannel::print(boost::format fmt)
{
	lock_guard<mutex> dbglock(m);

	std::cerr << boost::format("%1$09.3f %2%") % second_clock::local_time() % fmt << std::endl;
}
;

void CDebugChannel::logTo(string filename)
{
	lock_guard<mutex> dbglock(m);

	logfile.open(filename.c_str());

	if (logfile.fail())
		throw exception();

	ptime now = second_clock::local_time();

	logfile << endl << "Starting Log on " << to_simple_string(now) << endl;
}

/**
 * @brief Print an info message.
 */
void CDebugChannel::info(boost::format fmt)
{
	if (vlevel >= INFO)
		print(boost::format("INF: %1%") % fmt);

	log(boost::format("INF: %1%") % fmt);

}
;

/**
 * @brief Print an debug message.
 */
void CDebugChannel::debug(boost::format fmt)
{
	if (vlevel >= DEBUG)
		print(boost::format("DBG: %1%") % fmt);

	log(boost::format("DBG: %1%") % fmt);

}
;

/**
 * @brief Print an warning message.
 */
void CDebugChannel::warning(boost::format fmt)
{
	if (vlevel >= WARNING)
		print(boost::format("WRN: %1%") % fmt);

	log(boost::format("WRN: %1%") % fmt);

}
;

/**
 * @brief Print an error message.
 */
void CDebugChannel::error(boost::format fmt)
{
	if (vlevel >= ERROR)
		print(boost::format("ERR: %1%") % fmt);

	log(boost::format("ERR: %1%") % fmt);

}
;

/**
 * @brief Print an error message and terminate execution.
 */
void CDebugChannel::fail(boost::format fmt)
{
	print(boost::format("FAIL: %1%") % fmt);
	log(boost::format("FAIL: %1%") % fmt);
	logfile.close();
	lock_guard<mutex> dbglock(m);
	assert(false);
}
;

/*****************************************************************************/

/**
 * Constructor
 */
CSystemBoost::CSystemBoost(IDebugChannel::VerbosityLevel vlevel) :
		port(50779), nthreads(1), mainScheduler(NULL), nodearch(NULL)
{
	/// new io service
	io.reset(new boost::asio::io_service);

	/// build new debug channel
	chan.reset(new CDebugChannel(vlevel));
	/// set logfile
	chan->logTo("nena.log");
	/// assign as global
	globalChannel = this->getDebugChannel();
	/// print test message
	DBG_INFO("CSystemBoost alive");

	syncFactory.reset(new CBoostSyncFactory());

	daemonStartTime = boost::posix_time::microsec_clock::local_time();
}

CSystemBoost::CSystemBoost(IDebugChannel::VerbosityLevel vlevel, int nthreads) :
		port(50779), nthreads(nthreads), mainScheduler(NULL), nodearch(NULL)
{
	io.reset(new boost::asio::io_service);
	/// build new debug channel
	chan.reset(new CDebugChannel(vlevel));
	/// set logfile
	chan->logTo("nena.log");
	/// assign as global
	globalChannel = this->getDebugChannel();
	/// print test message
	DBG_INFO("CSystemBoost alive");

	syncFactory.reset(new CBoostSyncFactory());

	daemonStartTime = boost::posix_time::microsec_clock::local_time();
}

/**
 * Destructor
 */
CSystemBoost::~CSystemBoost()
{
	releaseNetAdapts();
	closeAppServers();
	schedulers.clear();
	DBG_DEBUG("CSystemBoost destroyed.");
}

void CSystemBoost::io_run()
{
	DBG_DEBUG("CSystemBoost: starts I/O...");
	io->run();
	DBG_DEBUG("CSystemBoost: ends I/O...");
}

void CSystemBoost::run()
{
	/// start i/o thread
	io_thread.reset(new boost::thread(boost::bind(&CSystemBoost::io_run, this)));

	/// start all schedulers
	list<shared_ptr<IMessageScheduler> >::iterator it;

	for (it = schedulers.begin(); it != schedulers.end(); it++)
		(*it)->run();
}

void CSystemBoost::stop()
{
	/// stop i/o thread
	DBG_INFO("CSystemBoost: Stopping io.");
	io->stop();
	DBG_INFO("CSystemBoost: Resetting io.");
	io->reset();
	DBG_INFO("CSystemBoost: Resetting io Thread.");
	io_thread.reset();

	/// stop schedulers
	list<shared_ptr<IMessageScheduler> >::iterator it;

	for (it = schedulers.begin(); it != schedulers.end(); it++)
		(*it)->stop();
}

void CSystemBoost::end()
{
	closeAppServers();
	releaseNetAdapts();
	schedulers.clear();
	io.reset();
}

/**
 * @brief	Initialize network accesses
 *
 * @param nodearch	Pointer to Daemon object
 * @param sched		Pointer to scheduler to use for Network Accesses
 */
void CSystemBoost::initNetAdapts(CNena * nodearch, IMessageScheduler * sched)
{
	shared_ptr<INetAdapt> na;
	shared_ptr<CBoostUDPNetAdapt> na_udp;

	if (nodearch->getConfig()->hasParameter(systemId, "netAdapts", XMLFile::STRING, XMLFile::TABLE)) {
		vector<vector<string> > nas;
		nodearch->getConfig()->getParameterTable(systemId, "netAdapts", nas);

		vector<vector<string> >::const_iterator it;
		for (it = nas.begin(); it != nas.end(); it++) {
			if (it->at(0) == "netadapt://boost/udp/" || it->at(0) == "netadapt://boost/udp") {
				na = shared_ptr<INetAdapt>(new CBoostUDPNetAdapt(nodearch, sched, it->at(1), io));
				if (na->isReady()) {
					netAdapts.push_back(na);

				} else {
					DBG_ERROR(FMT("NetAdapt not ready: %1%") % na->getId());

				}

			} else if (it->at(0) == "netadapt://boost/tap/" || it->at(0) == "netadapt://boost/tap") {
				na = shared_ptr<INetAdapt>(new CBoostTapNetAdapt(nodearch, sched, it->at(1), io));
				if (na->isReady()) {
					netAdapts.push_back(na);

				} else {
					DBG_ERROR(FMT("NetAdapt not ready: %1%") % na->getId());

				}

			} else if (it->at(0) == "netadapt://boost/raw/" || it->at(0) == "netadapt://boost/raw") {
				na = shared_ptr<INetAdapt>(new CBoostRawNetAdapt(nodearch, sched, it->at(1), io));
				if (na->isReady()) {
					netAdapts.push_back(na);

				} else {
					DBG_ERROR(FMT("NetAdapt not ready: %1%") % na->getId());

				}

			}

		}

	} else {
		// instantiate a default net adapt
		na_udp = shared_ptr<CBoostUDPNetAdapt>(new CBoostUDPNetAdapt(nodearch, sched, std::string(), io));
		na_udp->setArchId("architecture://edu.kit.tm/itm/simpleArch");
		na_udp->setProperty(INetAdapt::p_archid, new CStringValue(na_udp->getArchId()));
		na_udp->setProperty(INetAdapt::p_addr, new CStringValue((FMT("0.0.0.0:%1%") % port).str()));
		na_udp->setConfig();
		if (na_udp->isReady()) {
			netAdapts.push_back(na_udp);

		} else {
			DBG_ERROR(FMT("NetAdapt not ready: %1%") % na_udp->getId());

		}

	}

}

/**
 * @brief 	Return list of Network Accesses.
 *
 * @param netAdapts	List to which the Network Access will be appended
 */
void CSystemBoost::getNetAdapts(std::list<INetAdapt *> & netAdapts)
{
	for (std::list<shared_ptr<INetAdapt> >::iterator it = this->netAdapts.begin(); it != this->netAdapts.end(); it++)
		netAdapts.push_back(it->get());
}

/**
 * Release previously initialized network accesses
 */
void CSystemBoost::releaseNetAdapts()
{
	/// delete all netAdapts
	netAdapts.clear();
}

/**
 * @brief 	Initialize application interface
 *
 * @param nodearch	Pointer to Daemon object
 * @param sched		Pointer to scheduler to use for application interfaces
 */
void CSystemBoost::initAppServers(CNena * nodearch, IMessageScheduler * sched)
{
	if (nodearch->getConfig()->hasParameter(systemId, "appServersToLoad", XMLFile::STRING, XMLFile::LIST)) {
		// override default behavior
		vector<string> appServersToLoad;
		nodearch->getConfig()->getParameterList(systemId, "appServersToLoad", appServersToLoad);

		for (vector<string>::const_iterator it = appServersToLoad.begin(); it != appServersToLoad.end(); it++) {
			if (*it == "appServer://boost/sharedMemory")
				appServers.push_back(shared_ptr<IAppServer>(new CAppMemServer(nodearch, sched)));
			else if (*it == "appServer://boost/socket")
				appServers.push_back(shared_ptr<IAppServer>(new CAppSocketServer(nodearch, sched, io)));
		}

	} else {
		// default behavior: load socket server
		appServers.push_back(shared_ptr<IAppServer>(new CAppSocketServer(nodearch, sched, io)));

	}
}

void CSystemBoost::getAppServers(list<shared_ptr<IAppServer> > & as)
{
	as.insert(++as.begin(), appServers.begin(), appServers.end());
	DBG_INFO(FMT ("CSystemBoost: %1% appServers added, now %2% appServers total.") % appServers.size() % as.size ());
}

/**
 * Release / tear down application interface
 */
void CSystemBoost::closeAppServers()
{
	appServers.clear();
}

/**
 * @brief	Returns the system-specific main scheduler (e.g. main thread)
 */
IMessageScheduler * CSystemBoost::getMainScheduler() const
{
	return mainScheduler;
}

IMessageScheduler * CSystemBoost::lookupScheduler(IMessageProcessor * proc)
{
	boost::shared_lock<shared_mutex> sl(s_mutex);
	list<shared_ptr<IMessageScheduler> >::const_iterator it;

	for (it = schedulers.begin(); it != schedulers.end(); it++) {
		if ((*it)->hasMessageProcessor(proc))
			return it->get();
	}

	return NULL;
}

IMessageScheduler * CSystemBoost::getSchedulerByName(std::string name)
{
	boost::shared_lock<shared_mutex> sl(s_mutex);
	list<shared_ptr<IMessageScheduler> >::const_iterator it;

	for (it = schedulers.begin(); it != schedulers.end(); it++) {
		if ((*it)->getPrivateName() == name)
			return it->get();
	}

	return NULL;
}

void CSystemBoost::initSchedulers(CNena * na)
{
	int n = 1;

	nodearch = na;

	if (nodearch == NULL)
		throw ENotInitialized("CSystemBoost: schedulers not initialized yet!");

	shared_ptr<IMessageScheduler> s;

	if (nthreads != 0)
		n = nthreads;

	s.reset(new CBoostSchedulerMT(nodearch, io, n));

	schedulers.push_back(s);

	mainScheduler = s.get();

	/// init user defined schedulers

	if (nodearch->getConfig()->hasParameter(systemId, "schedulers")) {
		vector<string> schedulerList;
		nodearch->getConfig()->getParameterList(systemId, "schedulerList", schedulerList);

		for (vector<string>::const_iterator it = schedulerList.begin(); it != schedulerList.end(); it++) {
			s.reset(new CBoostSchedulerMT(nodearch, io, n, *it));
			schedulers.push_back(s);
		}

	}
}

/**
 * @brief	Returns a system-specific scheduler.
 */
IMessageScheduler * CSystemBoost::schedulerFactory()
{
	if (nodearch == NULL)
		throw ENotInitialized("CSystemBoost: schedulers not initialized yet!");

	shared_ptr<IMessageScheduler> s;

	int n = (boost::thread::hardware_concurrency() > 1) ? 2 : 1;

	s.reset(new CBoostSchedulerMT(nodearch, io, n));

	lock_guard<shared_mutex> lg(s_mutex);
	schedulers.push_back(s);

	return s.get();

}

ISyncFactory * CSystemBoost::getSyncFactory()
{
	return this->syncFactory.get();
}

/**
 * @brief Returns the system specific debug channel
 */
IDebugChannel * CSystemBoost::getDebugChannel()
{
	return dynamic_cast<IDebugChannel *>(chan.get());
}

/**
 * Return the name of the node
 */
string CSystemBoost::getNodeName() const
{
	return nodeName;
}

/**
 * Set the name of the node
 */
void CSystemBoost::setNodeName(std::string name)
{
	nodeName = name;
}

/**
 * Set the port for net Adapts
 */
void CSystemBoost::setNodePort(int p)
{
	port = p;
}

/**
 * @brief	Returns the system time in seconds (with at least milli-second
 * 			precision)
 *
 * 			TODO: untested
 */
double CSystemBoost::getSysTime()
{
	boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration diff = now - daemonStartTime;
	return ((double) diff.total_milliseconds()) / 1000;
}

/**
 * @brief	Returns a random number between [0, 1].
 */
double CSystemBoost::random()
{
	return ((double) rand()) / (double) RAND_MAX;
}

void CSystemBoost::setIcon(const std::string& iconName)
{

}

/*****************************************************************************/
