/** @file
 * flowState.h
 *
 * @brief Flow state class
 *
 * (c) 2008-2012 Institut fuer Telematik, KIT, Germany
 *
 *  Created on: Jul 06, 2012
 *      Author: denis
 */

#ifndef FLOWSTATE_H_
#define FLOWSTATE_H_

#include "messages.h"
#include "appConnector.h" // need to remove this dependency (IAppConnector::method_t)

#include <map>
#include <iostream>

/** maximum initial number of allowed packets within the stack */
#define FLOWSTATE_MAXFLOATINGPACKETS	16

#define FLOWSTATE_NOTIFICATION_EVENT "event://flowState/notification"

#define FLOWSTATE_STATISTICS_ID "flowState://statistics"

// dump floating packets history
//#define FLOWSTATE_FLOATINGPACKETS_HISTORY

// needed for performance when this should not be tracked
#ifdef FLOWSTATE_FLOATINGPACKETS_HISTORY
#define FLOWSTATE_FLOATOUT_INC(flowState, n, name, time) flowState->incOutFloatingPackets(n, name, time)
#define FLOWSTATE_FLOATOUT_DEC(flowState, n, name, time) flowState->decOutFloatingPackets(n, name, time)
#else
#define FLOWSTATE_FLOATOUT_INC(flowState, n, name, time) flowState->incOutFloatingPackets(n)
#define FLOWSTATE_FLOATOUT_DEC(flowState, n, name, time) flowState->decOutFloatingPackets(n)
#endif

/**
 * @brief	Flow state information.
 *
 * This is local information and only valid on this host.
 */
class CFlowState : public boost::enable_shared_from_this<CFlowState>
{
public:
	/**
	 * @brief	State object, can be inherited an specialised if desired.
	 */
	class StateObject : public boost::enable_shared_from_this<StateObject>
	{
	private:
		std::string objectId;
		nena::hash_t objectIdHash;

	protected:
		/**
		 * @brief	Set object URI.
		 */
		void setId(const std::string& uri)
		{
			objectId = uri;
			if (objectId.empty()) {
				objectIdHash = 0;

			} else {
				objectIdHash = nena::hash_string(objectId);

			}
		}

	public:
		StateObject() {};

		virtual ~StateObject()
		{
		};

		/**
		 * @brief	Return component URI. This might return an empty string.
		 */
		inline const std::string& getId() const
		{
			return objectId;
		}

		/**
		 * @brief	Safe cast (dynamic cast) to a subclass. Will not work reliably
		 * 			across shared libraries.
		 *
		 * @return 	Pointer to subclass or NULL if message is not a subclass of
		 * 			type T.
		 */
		template<class T>
		boost::shared_ptr<T> cast()
		{
			boost::shared_ptr<StateObject> so = shared_from_this();
			boost::shared_ptr<T> r = boost::dynamic_pointer_cast<T, StateObject>(so);
			return r;
		}
	};

	typedef enum {
		stat_packetCountIn,			///< total number of received packets
		stat_lossCountIn,			///< total number of losses detected at receiver
		stat_remote_packetCountIn,	///< total number of received packets
		stat_remote_lossCountIn,	///< total number of losses detected at receiver
		stat_remote_lossRate,		///< current (moving) loss rate
	} StatKeys;

	/**
	 * @brief	Preliminary statistics object
	 */
	class StatisticsObject : public StateObject
	{
	public:
		std::map<StatKeys, boost::shared_ptr<CMorphableValue> > values;

		StatisticsObject() {
			setId(FLOWSTATE_STATISTICS_ID);

			values[stat_packetCountIn].reset(new CIntValue(0));
			values[stat_lossCountIn].reset(new CIntValue(0));
			values[stat_remote_packetCountIn].reset(new CIntValue(0));
			values[stat_remote_lossCountIn].reset(new CIntValue(0));
			values[stat_remote_lossRate].reset(new CDoubleValue(0));
		}

		virtual ~StatisticsObject() {};

	};

	typedef enum {
		ev_none,
		ev_stateChanged,		///< State of flow state changed
		ev_flowControl			///< Flow control event
	} NotificationEvent;

	/**
	 * @brief	Event sent to listeners in case of floating packet credits are
	 * 			available again.
	 */
	class Notification : public IEvent
	{
	public:
		boost::shared_ptr<CFlowState> flowState;
		NotificationEvent event;

		Notification(boost::shared_ptr<CFlowState> flowState, NotificationEvent event,
				IMessageProcessor* from = NULL, IMessageProcessor* to = NULL) :
			IEvent(from, to), flowState(flowState), event(event)
		{}

		virtual const std::string getId() const
		{
			return FLOWSTATE_NOTIFICATION_EVENT;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			boost::shared_ptr<IEvent> ev(new Notification(flowState, event, from, to));
			return ev;
		}
	};

	typedef unsigned int FlowId;

	typedef enum {
		s_valid,	///< valid flow
		s_stale,	///< errorneous flow
		s_end		///< ended flow
	} OperationalState;

	typedef enum {
		e_none,
		e_reset,	///< reset by peer(s)
		e_timeout	///< flow timed out
	} ErrorState;

protected:
	std::map<std::string, boost::shared_ptr<StateObject> > stateObjects;
	std::list<IMessageProcessor*> listeners;

	IMessageProcessor* owner;
	FlowId flowId;
	FlowId remoteFlowId;
	OperationalState operationalState;
	ErrorState errorState;

	std::string serviceId; // service ID
	std::string localId; // local node name
	std::string remoteId; // remote node name
	IAppConnector::method_t method; // method (outgoing requests)

	std::string requestUri; // request URI
	IAppConnector::method_t requestMethod; // request method (incoming requests)

	std::list<boost::shared_ptr<CFlowState> > childFlowStates;	// TODO: use a better data structure

	/* flow control */
	unsigned int inFloatingPackets; 	///< number of packets currently floating in stack
	unsigned int inMaxFloatingPackets;	///< maximum allowed number of packets floating within the stack
	unsigned int outFloatingPackets; 	///< number of packets currently floating in stack
	unsigned int outMaxFloatingPackets;	///< maximum allowed number of packets floating within the stack

#ifdef FLOWSTATE_FLOATINGPACKETS_HISTORY
	std::list<unsigned int> outFloatingPacketsHistory;
	std::list<unsigned int> outMaxFloatingPacketsHistory;
	std::list<std::string> outFloatingPacketsWhoHistory;
	std::list<double> outFloatingPacketsTimeHistory;
#endif

	/** additional properties */
	std::map<IMessage::PropertyId, boost::shared_ptr<CMorphableValue> > properties;

public:
	/**
	 * @brief	NENA internal, do not create one yourself. Use
	 * 			CNetletSelector::createFlow() instead.
	 */
	CFlowState(FlowId flowId, IMessageProcessor* owner) :
			owner(owner), flowId(flowId), remoteFlowId(0), operationalState(s_valid),
			errorState(e_none), method(IAppConnector::method_none), requestMethod(IAppConnector::method_none),
			inFloatingPackets(0), inMaxFloatingPackets(FLOWSTATE_MAXFLOATINGPACKETS),
			outFloatingPackets(0), outMaxFloatingPackets(FLOWSTATE_MAXFLOATINGPACKETS)
	{
		boost::shared_ptr<StateObject> stat(new StatisticsObject());
		addStateObject(FLOWSTATE_STATISTICS_ID, stat);
	}

	virtual ~CFlowState()
	{
#ifdef FLOWSTATE_FLOATINGPACKETS_HISTORY
		std::string filename((FMT("flow%1%_outFloating") % flowId).str());
		std::ofstream out(filename.c_str());

		out << "# outFloatingPacketsHistory \toutMaxFloatingPacketsHistory" << std::endl;

		double offset = outFloatingPacketsTimeHistory.front();
		std::list<unsigned int>::const_iterator it1;
		std::list<unsigned int>::const_iterator it2;
		std::list<std::string>::const_iterator it3;
		std::list<double>::const_iterator it4;
		for (it1 = outFloatingPacketsHistory.begin(), it2 = outMaxFloatingPacketsHistory.begin(), it3 = outFloatingPacketsWhoHistory.begin(), it4 = outFloatingPacketsTimeHistory.begin();
			it1 != outFloatingPacketsHistory.end(), it2 != outMaxFloatingPacketsHistory.end(), it3 != outFloatingPacketsWhoHistory.end(), it4 != outFloatingPacketsTimeHistory.end();
			it1++, it2++, it3++, it4++)
		{
			out << (*it4 - offset) << "\t" << *it1 << "\t" << *it2 << "\t" << *it3 << std::endl;
		}
#endif // FLOWSTATE_FLOATINGPACKETS_HISTORY

		DBG_INFO(FMT("Destroyed flow state (ID %1%)") % flowId);
	}

	virtual void cleanUp()
	{
		stateObjects.clear();
	}

	/**
	 * @brief	Owner of flow state
	 */
	inline IMessageProcessor* getOwner()
	{
		return owner;
	}

	/**
	 * @brief	Returns a flow ID which is unique on this host and identifies
	 * 			this flow.
	 */
	inline FlowId getFlowId()
	{
		return flowId;
	}

	/**
	 * @brief	Returns operational state.
	 */
	inline OperationalState getOperationalState()
	{
		return operationalState;
	}

	/**
	 * @brief	Sets operational state.
	 *
	 * @param sender	Sender of state changed event
	 * @param state		State to be set
	 */
	inline void setOperationalState(IMessageProcessor* sender, OperationalState state)
	{
		operationalState = state;
		if (sender != NULL)
			notify(sender, ev_stateChanged);
	}

	/**
	 * @brief	Returns error state.
	 */
	inline ErrorState getErrorState()
	{
		return errorState;
	}

	/**
	 * @brief	Sets error state.
	 *
	 * @param sender	Sender of state changed event
	 * @param state		State to be set
	 */
	inline void setErrorState(IMessageProcessor* sender, ErrorState state)
	{
		errorState = state;
//		if (sender != NULL)
//			notify(sender, ev_stateChanged);
	}

	/**
	 * @brief	Returns remote flow ID (may be 0)
	 */
	inline FlowId getRemoteFlowId()
	{
		return remoteFlowId;
	}

	/**
	 * @brief	Sets remote flow ID (may be 0)
	 */
	inline void setRemoteFlowId(FlowId remoteFlowId)
	{
		this->remoteFlowId = remoteFlowId;
	}

	/**
	 * @brief	Returns service URI
	 */
	inline std::string getServiceId()
	{
		return serviceId;
	}

	/**
	 * @brief	Sets service URI
	 */
	inline void setServiceId(std::string serviceId)
	{
		this->serviceId = serviceId;
	}

	/**
	 * @brief	Returns local node name
	 */
	inline std::string getLocalId()
	{
		return localId;
	}

	/**
	 * @brief	Sets local node name
	 */
	inline void setLocalId(std::string localId)
	{
		this->localId = localId;
	}

	/**
	 * @brief	Returns remote node name
	 */
	inline std::string getRemoteId()
	{
		return remoteId;
	}

	/**
	 * @brief	Sets remote node name
	 */
	inline void setRemoteId(std::string remoteId)
	{
		this->remoteId = remoteId;
	}

	/**
	 * @brief	Returns method
	 */
	inline IAppConnector::method_t getMethod()
	{
		return method;
	}

	/**
	 * @brief	Sets method
	 */
	inline void setMethod(IAppConnector::method_t method)
	{
		this->method = method;
	}

	/**
	 * @brief	Returns requested URI or service URI
	 */
	inline std::string getRequestUri()
	{
		if (requestUri.empty())
			return this->serviceId;
		return requestUri;
	}

	/**
	 * @brief	Sets requested URI
	 */
	inline void setRequestUri(std::string requestUri)
	{
		this->requestUri = requestUri;
	}

	/**
	 * @brief	Returns requested method
	 */
	inline IAppConnector::method_t getRequestMethod()
	{
		return requestMethod;
	}

	/**
	 * @brief	Sets requested method
	 */
	inline void setRequestMethod(IAppConnector::method_t requestMethod)
	{
		this->requestMethod = requestMethod;
	}

	/**
	 * @brief	Adds a flow state as a child of this flow state
	 */
	inline void addChildFlowState(boost::shared_ptr<CFlowState> flowState)
	{
		childFlowStates.push_back(flowState);
	}

	/**
	 * @brief	Adds a flow state as a child of this flow state
	 */
	inline void removeChildFlowState(boost::shared_ptr<CFlowState> flowState)
	{
		std::list<boost::shared_ptr<CFlowState> >::iterator it;
		for (it = childFlowStates.begin(); it != childFlowStates.end(); it++) {
			if (*it == flowState) {
				childFlowStates.erase(it);
				break;
			}
		}
	}

	/**
	 * @brief	Look for a child flow (may return NULL)
	 */
	inline boost::shared_ptr<CFlowState> getChildFlowState(const std::string& remoteId, FlowId remoteFlowId)
	{
		boost::shared_ptr<CFlowState> fs;
		std::list<boost::shared_ptr<CFlowState> >::iterator it;
		for (it = childFlowStates.begin(); it != childFlowStates.end(); it++) {
			if (((*it)->getRemoteId() == remoteId) and ((*it)->getRemoteFlowId() == remoteFlowId)) {
				fs = *it;
				break;
			}
		}
		return fs;
	}

	/**
	 * @brief	Return state object with the given name (should be the URI of
	 * 			the entity which uses the object).
	 */
	boost::shared_ptr<StateObject> getStateObject(const std::string& objectName)
	{
		std::map<std::string, boost::shared_ptr<StateObject> >::const_iterator it;
		it = stateObjects.find(objectName);
		if (it != stateObjects.end()) {
			return it->second;

		} else {
			return boost::shared_ptr<StateObject>();

		}
	}

	/**
	 * @brief	Adds a given state object. objectName should be the URI of the
	 * 			entity which uses the object. It is an error to add two state
	 * 			objects with the same name.
	 */
	virtual void addStateObject(const std::string& objectName, boost::shared_ptr<StateObject> object)
	{
		assert(!objectName.empty());

		std::map<std::string, boost::shared_ptr<StateObject> >::const_iterator it;
		it = stateObjects.find(objectName);
		assert(it == stateObjects.end());

		stateObjects[objectName] = object;
	}

	/**
	 * @brief	Removes a state object permanently from this flow state
	 */
	virtual void removeStateObject(const std::string& objectName)
	{
		std::map<std::string, boost::shared_ptr<StateObject> >::iterator it;
		it = stateObjects.find(objectName);
		if (it != stateObjects.end()) {
			stateObjects.erase(it);

		} else {
			// if you arrive here, you probably should reconsider your design
			assert(false);

		}
	}

	/**
	 * @brief	Return number of incoming floating packets within the stack.
	 * 			This is for flow control.
	 */
	inline unsigned int getInFloatingPackets()
	{
		// TODO add thread-safety
		return inFloatingPackets;
	}

	/**
	 * @brief	Increase number of floating packets within the stack. This has
	 * 			to be called for every packet generated or duplicated. This is
	 * 			for flow control.
	 */
	inline void incInFloatingPackets(unsigned int n = 1)
	{
		// TODO add thread-safety
		inFloatingPackets += n;
	}

	/**
	 * @brief	Decrease number of floating packets within the stack. This has
	 * 			to be called for every packet consumed/destroyed by the stack.
	 * 			This is for flow control.
	 */
	inline void decInFloatingPackets(unsigned int n = 1)
	{
		// TODO add thread-safety
		if (inFloatingPackets >= n)
			inFloatingPackets -= n;
	}

	/**
	 * @brief	Return number of maximum allowed incoming floating packets
	 *			within the stack. This is for flow control.
	 */
	inline unsigned int getInMaxFloatingPackets()
	{
		// TODO add thread-safety
		return inMaxFloatingPackets;
	}

	/**
	 * @brief	Set maximum allowed number of floating packets within the stack.
	 * 			This is for flow control.
	 */
	inline void setInMaxFloatingPackets(unsigned int n)
	{
		// TODO add thread-safety
		// TODO verify min/max in case of multiple message processors limiting this number
		inMaxFloatingPackets = n;
	}

	/**
	 * @brief	Check whether we can receive incoming packets
	 */
	inline bool canReceiveIncomingPackets()
	{
		// TODO thread-safety
		return inFloatingPackets < inMaxFloatingPackets;
	}

	/**
	 * @brief	Return number of outgoing floating packets within the stack.
	 * 			This is for flow control.
	 */
	inline unsigned int getOutFloatingPackets()
	{
		// TODO add thread-safety
		return outFloatingPackets;
	}

	/**
	 * @brief	Increase number of floating packets within the stack. This has
	 * 			to be called for every packet generated or duplicated. This is
	 * 			for flow control.
	 */
#ifdef FLOWSTATE_FLOATINGPACKETS_HISTORY
	inline void incOutFloatingPackets(unsigned int n = 1, const std::string& who = std::string(), double time = 0.0)
#else
	inline void incOutFloatingPackets(unsigned int n = 1)
#endif
	{
		// TODO add thread-safety
		outFloatingPackets += n;
#ifdef FLOWSTATE_FLOATINGPACKETS_HISTORY
		outFloatingPacketsHistory.push_back(outFloatingPackets);
		outMaxFloatingPacketsHistory.push_back(outMaxFloatingPackets);
		outFloatingPacketsWhoHistory.push_back(who);
		outFloatingPacketsTimeHistory.push_back(time);
#endif
	}

	/**
	 * @brief	Decrease number of floating packets within the stack. This has
	 * 			to be called for every packet consumed/destroyed by the stack.
	 * 			This is for flow control.
	 */
#ifdef FLOWSTATE_FLOATINGPACKETS_HISTORY
	inline void decOutFloatingPackets(unsigned int n = 1, const std::string& who = std::string(), double time = 0.0)
#else
	inline void decOutFloatingPackets(unsigned int n = 1)
#endif
	{
		// TODO add thread-safety
		if (outFloatingPackets >= n)
			outFloatingPackets -= n;
#ifdef FLOWSTATE_FLOATINGPACKETS_HISTORY
		outFloatingPacketsHistory.push_back(outFloatingPackets);
		outMaxFloatingPacketsHistory.push_back(outMaxFloatingPackets);
		outFloatingPacketsWhoHistory.push_back(who);
		outFloatingPacketsTimeHistory.push_back(time);
#endif
	}

	/**
	 * @brief	Return number of maximum allowed outgoing floating packets
	 * 			within the stack. This is for flow control.
	 */
	inline unsigned int getOutMaxFloatingPackets()
	{
		// TODO add thread-safety
		return outMaxFloatingPackets;
	}

	/**
	 * @brief	Set maximum allowed number of floating packets within the stack.
	 * 			This is for flow control.
	 */
	inline void setOutMaxFloatingPackets(unsigned int n)
	{
		// TODO add thread-safety
		// TODO verify min/max in case of multiple message processors limiting this number
		outMaxFloatingPackets = n;
	}

	/**
	 * @brief	Check whether we can send outgoing packets
	 */
	inline bool canSendOutgoingPackets()
	{
		// TODO thread-safety
		return outFloatingPackets < outMaxFloatingPackets;
	}

	/**
	 * @brief	Register a listener for events on this flow state.
	 */
	inline void registerListener(IMessageProcessor* mp)
	{
		// TODO thread-safety
		listeners.push_back(mp);
	}

	/**
	 * @brief	Unregister a listener for events on this flow state.
	 */
	inline void unregisterListener(IMessageProcessor* mp)
	{
		// TODO thread-safety
		std::list<IMessageProcessor*>::iterator it;
		for (it = listeners.begin(); it != listeners.end(); it++) {
			if (*it == mp) {
				listeners.erase(it);
				return;
			}
		}
	}

	/**
	 * @brief	Notify listeners
	 */
	virtual void notify(IMessageProcessor* sender, NotificationEvent event)
	{
		// TODO thread-safety
		if (canSendOutgoingPackets()) {
			std::list<IMessageProcessor*>::iterator it;
			for (it = listeners.begin(); it != listeners.end(); it++) {
				boost::shared_ptr<Notification> ev(new Notification(shared_from_this(), event, sender, *it));
				sender->getMessageScheduler()->sendMessage(ev);
			}
		}
	}

	/**
	 * @brief	Safely set a property, erasing the old one if necessary.
	 */
	inline void setProperty(const IMessage::PropertyId pid, boost::shared_ptr<CMorphableValue> val)
	{
		std::map<IMessage::PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			if (val.get() == 0) {
				properties.erase(pit);
				return;
			}

		}

		if (val.get() != 0)
			properties[pid] = val;
	}

	/**
	 * @brief	Safely return a property.
	 *
	 * 			As template parameter, use the effective value type, e.g.
	 * 			int, string, etc. Use CMorphableValue getProperty() if you
	 * 			do not know the effective value type.
	 *
	 * @throw	IMessage::EPropertyNotDefined if the property is not set.
	 * @throw	CMorphableValue::EValueTypeMismatch if the given value type
	 * 			does not match the effective value type.
	 *
	 * @return	The property value.
	 */
	template<typename T>
	boost::shared_ptr<T> getProperty(const IMessage::PropertyId pid) throw (IMessage::EPropertyNotDefined,
			CMorphableValue::EValueTypeMismatch)
	{
		std::map<IMessage::PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			boost::shared_ptr<T> r = pit->second->cast<T>();
			return r;

		} else {
			throw IMessage::EPropertyNotDefined();

		}
	}

};

#endif // FLOWSTATE_H_
