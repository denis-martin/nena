/** @file
 * enhancedAppConnector.h
 *
 * @brief App Connector Interface with shared functions implemented
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Mar 22, 2009
 *      Author: Benjamin Behringer
 */

#ifndef ENHANCEDAPPCONNECTOR_H_
#define ENHANCEDAPPCONNECTOR_H_

#include "appConnector.h"
#include "nena.h"
#include "netletSelector.h"
#include "msg.h"
#include "messageBuffer.h"
#include "debug.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <boost/enable_shared_from_this.hpp>


#define 	KB 		*1024
#define 	MB		*1024 KB
#define		GB		*1024 MB

class IEnhancedAppConnector : public IAppConnector, public boost::enable_shared_from_this<IEnhancedAppConnector>
{
protected:
	CNena * nodearch;			///< Pointer to Node Architecture, need this to register
	IAppServer * appServer;		///< Pointer to server which spawned the connection

	CNetletSelector* netletSelector;	///< Pointer to Netlet selector

	bool connectorTest; 		///< send packets right back to the app
	bool extConnectorTest;		///< send packets back to the app using scheduler

	bool appRecvBlocked;		///< whether reading from application is currently blocked

	class UserRequest
	{
	public:
		IMessageProcessor* replyTo;
		uint32_t requestId;

		UserRequest(IMessageProcessor* replyTo = NULL, uint32_t requestId = 0) :
			replyTo(replyTo), requestId(requestId)
		{};
	};

	/**
	 * @brief	Timer after which the app connector class gets destroyed after
	 *			it is released
	 */
	class ReleaseTimer : public CTimer
	{
	public:
		ReleaseTimer(IMessageProcessor *proc, double delay = 10)
			: CTimer(delay, proc) {}
		virtual ~ReleaseTimer() {}
	};

	uint32_t userRequestId;		///< User request ID
	std::map<uint32_t, UserRequest> userRequests;	///< Pending user requests

	/**
	 * @brief takes payload from the app and handles it
	 */
	virtual void handlePayload (MSG_TYPE type, shared_buffer_t payload);

	/**
	 * @brief takes payload from nodearch and sends it to app
	 */
	virtual void sendPayload (MSG_TYPE type, shared_buffer_t payload) = 0;

public:
	IEnhancedAppConnector (IMessageScheduler* sched, CNena * na, IAppServer * server);
	virtual ~IEnhancedAppConnector ();

	/// helper functions for registration at the nodearch
	void registerMe();
	void unregisterMe();

	/**
	 * @brief	Switch connector Test on/off, will send messages directly back to the app without invoking the scheduler
	 */
	virtual void setConnectorTest (bool test) { connectorTest = test; }

	/**
	 * @brief	get connector test status
	 */
	virtual bool getConnectorTest () const { return connectorTest; }

	/**
	 * @brief	Switch connector Test on/off, will send messages directly back to the app, using the scheduler
	 */
	virtual void setExtConnectorTest (bool test) { extConnectorTest = test; }

	/**
	 * @brief	get extended connector test status
	 */
	virtual bool getExtConnectorTest () const { return extConnectorTest; }

	/**
	 * @brief start all i/o on this app connector
	 */
	virtual void start () = 0;

	/**
	 * @brief stop all i/o on this app connector
	 */
	virtual void stop () = 0;

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an incoming message directed towards the application.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief	Present installation request to the user
	 *
	 * @param msgstr		String to present to the user
	 * @param requestType	Sum/arithmetic OR of reply options, see IAppConnector::userreq_t
	 * @param replyTo		Message processor to reply to
	 * @param requestId		Optional request ID
	 */
	virtual void sendUserRequest(const std::string& msgstr, userreq_t requestType, IMessageProcessor *replyTo, uint32_t requestId);

	/**
	 * @brief	Send USER_1 signal; temporarily added
	 *
	 * @param message 	User message
	 */
	virtual void sendUser1(boost::shared_ptr<CMessageBuffer> message);

	/**
	 * @brief	Send an event to the application
	 *
	 * @param event		Event ID
	 * @param param		Parameter for the event
	 */
	virtual void sendEvent(event_t event, const std::string& param);

	/**
	 * @brief	Release / close app connector
	 */
	virtual void release();
};

class IEnhancedAppServer : public IAppServer
{
protected:
	/// list of spawned AppConnectors
	std::set<boost::shared_ptr<IEnhancedAppConnector> > spawnedAppConnectors;
	std::set<boost::shared_ptr<IEnhancedAppConnector> > releasedAppConnectors;
	/// mutex protecting the set
	boost::mutex mset;

public:
	IEnhancedAppServer (IMessageScheduler* sched):
	IAppServer (sched)
	{ className += "::IEnhancedAppServer"; }
	virtual ~IEnhancedAppServer () {}

	/**
	 * 	@brief called by a AppConnector when its connection dies
	 */
	virtual void notifyEnd (boost::shared_ptr<IEnhancedAppConnector> dyingCon)
	{
		dyingCon->stop ();

		boost::lock_guard<boost::mutex> set_guard(mset);
		spawnedAppConnectors.erase (dyingCon);
		releasedAppConnectors.insert(dyingCon);
	}

	/**
	 * 	@brief Called after a certain timeout when the AppConnector got released
	 */
	virtual void destroyAppConnector (boost::shared_ptr<IEnhancedAppConnector> dyingCon)
	{
		boost::lock_guard<boost::mutex> set_guard(mset);
		releasedAppConnectors.erase(dyingCon);
	}

	virtual void addNew (boost::shared_ptr<IEnhancedAppConnector> newCon)
	{
		boost::lock_guard<boost::mutex> set_guard(mset);
		spawnedAppConnectors.insert (newCon);
	}

	virtual void startAppConnectors ()
	{
		DBG_INFO(boost::format("%1%: starting all App Connectors!") % getId());
		boost::lock_guard<boost::mutex> set_guard(mset);
		for (std::set<boost::shared_ptr<IEnhancedAppConnector> >::iterator it = spawnedAppConnectors.begin(); it != spawnedAppConnectors.end(); it++)
			(*it)->start ();
	}

	virtual void stopAppConnectors ()
	{
		DBG_INFO(boost::format("%1%: stopping all App Connectors!") % getId());
		boost::lock_guard<boost::mutex> set_guard(mset);
		for (std::set<boost::shared_ptr<IEnhancedAppConnector> >::iterator it = spawnedAppConnectors.begin(); it != spawnedAppConnectors.end(); it++)
			(*it)->stop ();
	}
};

#endif
