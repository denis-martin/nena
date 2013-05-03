/** @file
 * netletSelector.h
 *
 * @brief Netlet selection implementation.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Feb 12, 2009
 *      Author: denis
 */

#ifndef NETLETSELECTOR_H_
#define NETLETSELECTOR_H_

#include "messages.h"
#include "flowState.h"
#include "properties.h"
#include "appConnector.h"
#include "nameAddrMapper.h"

#include "xmlNode/xmlNode.h"

#include <map>
#include <list>

#include <boost/thread/shared_mutex.hpp>
#include <pugixml.h>
#include <exceptions.h>

// TODO: find a better solution to handle event IDs
#define EVENT_NETLETSELECTOR_NETLETCHANGED			"event://netletSelector/NetletChanged"
#define EVENT_NETLETSELECTOR_NOSUITABLENETLET		"event://netletSelector/NoSuitableNetlet"
#define EVENT_NETLETSELECTOR_APPSERVICEREGISTERED	"event://netletSelector/AppServiceRegistered"
#define EVENT_NETLETSELECTOR_APPSERVICEUNREGISTERED	"event://netletSelector/AppServiceUnregistered"
#define EVENT_NETLETSELECTOR_APPCONNREADY 			"event://netletSelector/AppConnReady"

/**
 * Sent to a Netlet when a new application connects to NENA. In addition, this
 * is emitted to all listeners.
 */
#define EVENT_NETLETSELECTOR_APPCONNECT				"event://netletSelector/AppConnect"

/**
 * Sent to a Netlet when an associated application disconnects from NENA. In
 * addition, this is emitted to all listeners.
 */
#define EVENT_NETLETSELECTOR_APPDISCONNECT			"event://netletSelector/AppDisconnect"

class CNena;

/**
 * @brief	Manages application connections and implements Netlet selection
 * 			functionality.
 */
class CNetletSelector : public IMessageProcessor
{
public:
	NENA_EXCEPTION(EConfig);

	class Event_NetletChanged : public IEvent
	{
	public:
		IAppConnector* appConn;

		Event_NetletChanged(IAppConnector* appConn = NULL, IMessageProcessor *from = NULL, IMessageProcessor *to = NULL) :
				IEvent(from, to), appConn(appConn)
		{
		}

		virtual const std::string getId() const
		{
			return EVENT_NETLETSELECTOR_NETLETCHANGED;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(new Event_NetletChanged(appConn, from, to));
		}
	};

	class Event_NoSuitableNetlet : public IEvent
	{
	public:
		IAppConnector* appConn;

		Event_NoSuitableNetlet(IAppConnector* appConn = NULL, IMessageProcessor *from = NULL, IMessageProcessor *to =
				NULL) :
				IEvent(from, to), appConn(appConn)
		{
		}

		virtual const std::string getId() const
		{
			return EVENT_NETLETSELECTOR_NOSUITABLENETLET;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(new Event_NoSuitableNetlet(appConn, from, to));
		}
	};

	class Event_AppServiceRegistered : public IEvent
	{
	public:
		std::string identifier;
		void* requirements;
		boost::shared_ptr<CFlowState> flowState;

		Event_AppServiceRegistered(std::string identifier = std::string(), void* requirements = NULL,
				boost::shared_ptr<CFlowState> flowState = boost::shared_ptr<CFlowState>(), IMessageProcessor *from =
						NULL, IMessageProcessor *to = NULL) :
				IEvent(from, to), identifier(identifier), requirements(requirements), flowState(flowState)
		{
		}

		virtual const std::string getId() const
		{
			return EVENT_NETLETSELECTOR_APPSERVICEREGISTERED;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(
					new Event_AppServiceRegistered(identifier, requirements, flowState, from, to));
		}
	};

	class Event_AppServiceUnregistered : public IEvent
	{
	public:
		std::string identifier;
		void* requirements;
		boost::shared_ptr<CFlowState> flowState;

		Event_AppServiceUnregistered(std::string identifier = std::string(), void* requirements = NULL,
				boost::shared_ptr<CFlowState> flowState = boost::shared_ptr<CFlowState>(), IMessageProcessor *from =
						NULL, IMessageProcessor *to = NULL) :
				IEvent(from, to), identifier(identifier), requirements(requirements), flowState(flowState)
		{
		}

		virtual const std::string getId() const
		{
			return EVENT_NETLETSELECTOR_APPSERVICEUNREGISTERED;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(
					new Event_AppServiceUnregistered(identifier, requirements, flowState, from, to));
		}
	};

	/**
	 * Sent to a Netlet when a new application connects to NENA. In addition, this
	 * is emitted to all listeners.
	 */
	class Event_AppConnect : public IEvent
	{
	public:
		IAppConnector::method_t method;
		std::string requestUri;

		Event_AppConnect(boost::shared_ptr<CFlowState> flowState,
				IAppConnector::method_t method,
				std::string requestUri,
				IMessageProcessor *from = NULL,
				IMessageProcessor *to = NULL) :
				IEvent(from, to), method(method), requestUri(requestUri)
		{
			setFlowState(flowState);
		}

		virtual const std::string getId() const
		{
			return EVENT_NETLETSELECTOR_APPCONNECT;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(new Event_AppConnect(*this));
		}
	};

	/**
	 * Sent to a Netlet when an associated application disconnects from NENA. In
	 * addition, this is emitted to all listeners.
	 */
	class Event_AppDisconnect : public IEvent
	{
	public:
		boost::shared_ptr<CFlowState> flowState;

		Event_AppDisconnect(boost::shared_ptr<CFlowState> flowState = boost::shared_ptr<CFlowState>(),
				IMessageProcessor *from = NULL, IMessageProcessor *to = NULL) :
				IEvent(from, to), flowState(flowState)
		{
		}

		virtual const std::string getId() const
		{
			return EVENT_NETLETSELECTOR_APPDISCONNECT;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(
					new Event_AppDisconnect(flowState, from, to));
		}
	};

	/**
	 * Sent to an app connector when it is ready to send data.
	 */
	class Event_AppConnReady : public IEvent
	{
	public:
		boost::shared_ptr<CFlowState> flowState;

		Event_AppConnReady(boost::shared_ptr<CFlowState> flowState = boost::shared_ptr<CFlowState>(),
				IMessageProcessor *from = NULL, IMessageProcessor *to = NULL) :
				IEvent(from, to), flowState(flowState)
		{
		}

		virtual const std::string getId() const
		{
			return EVENT_NETLETSELECTOR_APPCONNREADY;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(
					new Event_AppConnReady(flowState, from, to));
		}
	};

protected:
	CNena* nena;
	IMessageProcessor* management;

	boost::shared_mutex appConnMutex;	///< protects access to appConn-related containers

	std::map<CFlowState::FlowId, IAppConnector*> appConns;	///< connection id -> connector
	std::map<std::string, IAppConnector*> services;			///< published application service -> app connector
	std::list<INameAddrMapper*> nameAddrMappers;			///< list of known name/addr mappers

	std::map<std::string, IAppConnector *> parentAppConns;
	std::map<std::string, std::list<boost::shared_ptr<IMessage> > > pendingRequests;

	CWeightedProperty* systemPolicies;					///< collection of system policies for Netlet selection

	virtual void parseRequirementNode(sxml::XmlNode* reqNode, CWeightedProperty* parent);
	virtual void refreshSystemPolicies();

public:
	CNetletSelector(CNena* nodeA, IMessageScheduler *sched);	///< Constructor
	virtual ~CNetletSelector();												///< Destructor

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

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
	 * @brief 	Register a name/addr mapper that will be consulted in case an
	 * 			app connector has no Netlet associated with it yet.
	 */
	virtual void registerNameAddrMapper(INameAddrMapper* mapper);

	/**
	 * @brief 	Unregister a name/addr mapper.
	 */
	virtual void unregisterNameAddrMapper(INameAddrMapper* mapper);

	/**
	 * @brief	Register an app connector. Usually called by wrapper
	 * 			implementations. A new flow state will be assigned.
	 */
	virtual void registerAppConnector(IAppConnector* appConn); // "socket"

	/**
	 * @brief	Unregister an app connector. Usually called by wrapper
	 * 			implementations.
	 */
	virtual void unregisterAppConnector(IAppConnector* appConn); // "socket"

	/**
	 * @brief	Assigns the given Netlet to the given app connector. If the
	 * 			Netlet was not assigned before, an Event_NetletChanged is
	 * 			emitted.
	 */
	virtual void selectNetlet(IAppConnector* appc, INetlet* netlet);

	/**
	 * @brief	Implements automatic Netlet selection. If the Netlet was not
	 * 			assigned before, an Event_NetletChanged is emitted.
	 *
	 * @param appConn	Application connector for which the Netlet will be selected
	 */
	virtual void selectNetlet(IAppConnector* appc);

	/**
	 * @brief 	Register an application service.
	 * 			Event_AppServiceRegistered will be emitted.
	 */
	virtual void registerAppService(const std::string & serviceId, IAppConnector* appConn);

	/**
	 * @brief 	Unregister an application service.
	 * 			Event_AppServiceUnregistered will be emitted.
	 */
	virtual void unregisterAppService(const std::string & serviceId, IAppConnector* appConn);

	/**
	 * @brief	Lookup a published service ID and return a local flow ID which
	 * 			identifies the application connector (thus, the application providing
	 * 			the service)
	 *
	 * @param	serviceId	String identifying the service
	 *
	 * @returns	Local flow ID of an application providing the service
	 */
	virtual CFlowState::FlowId lookupAppService(std::string serviceId) const;

	/**
	 * @brief	Returns the app connector for the given flow ID.
	 */
	virtual IAppConnector* lookupFlowId(CFlowState::FlowId flowId) const;

	/**
	 * @brief	Fills the provided list with known service IDs. The list will be
	 * 			cleared if it is not empty.
	 *
	 * @param	services	Reference to the list that is to be filled
	 */
	virtual void getRegisteredAppServices(std::list<std::string>& services) const;

	virtual void getActiveApps(std::list<IAppConnector*>& appConns);
};

#endif /* NETLETSELECTOR_H_ */
