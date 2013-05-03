/** @file
 * message.h
 *
 * @brief Generic classes for message passing within the same address space
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jul 13, 2009
 *      Author: denis
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

// define this flag to print message traces
//#define DEBUG_MESSAGES_TRACE

// define this flag to enable loop-detection
#define DEBUG_MESSAGES_LOOPDETECTION

// define this flag to enable performance profiling of message processors
//#define DEBUG_MESSAGES_PROFILE

#include "debug.h"
#include "morphableValue.h"

// for NULL definition
#include <stddef.h>

#ifdef DEBUG_MESSAGES_PROFILE
// perf test
#include <sys/time.h>
#include "stat.h"
#endif // DEBUG_MESSAGES_PROFILE
#include <list>
#include <map>
#include <exception>
#include <exceptions.h>
#include <string>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

class IMessageProcessor;
class IMessageScheduler;
class CNena;
class CFlowState;

namespace nena
{
typedef unsigned int hash_t;		///< Default NENA hash type

/**
 * @brief	Default NENA hash function.
 *
 * hash_type:	integer compatible
 * vector_type:	needs to support
 *	 				.size() 	(returning const std::size_t) and
 *		 			operator[]	(returning char or unsigned char)
 */
template<typename hash_type, class vector_type>
hash_type hash_template(const vector_type& data)
{
	// compute ELF hash
	hash_type h = 0, g;
	for (std::size_t i = 0; i < data.size(); i++) {
		h = (h << 4) + static_cast<unsigned char>(data[i]);
		g = h & 0xf0000000L;
		if (g != 0)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

inline hash_t hash_string(std::string s)
{
	return hash_template<nena::hash_t, std::string>(s);
}
}

/**
 * @brief	Generic message interface.
 *
 * A message can be a packet, a local event, or a timer.
 */
class IMessage : public boost::enable_shared_from_this<IMessage>
{
public:
	enum Type
	{
		t_none = 0, t_timer, t_event, t_message,		// undirected message
		t_incoming,		// sent to default "previous" message processor
		t_outgoing,		// sent to default "next" message processor
		t_max
	};

	enum PropertyId
	{
		p_none = 0, p_flowState,		// CFlowState pointer

		// need to get rid of those and just use one src/dest
		p_destId,			// destination identifier (string)
		p_srcId,			// source identifier (string)
		p_destLoc,			// destination locator
		p_srcLoc,			// source locator
		p_multiDestLoc,		// multiple destination locators (e.g. for multicast)
		p_nextHopLoc,		// next hop locator

		// need to get rid of those (use flow state instead)
		p_netletId,			// Netlet identifier (string)
		p_serviceId,		// service ID (string)

		p_netAdapt,			// pointer to network adaptor class the message came from / is to be sent to
		p_autoForward,// if false the packet gets delivered on every intermediate node where it arrives (defaults to true)
		p_optString,        // option string containing meta data
		p_method,			// method (command mode, IAppConnector::method_t)
		p_endOfStream,		// end of message stream (bool)
		p_userBase = 1000
	};

	/**
	 * @brief	Thrown if the expected IMessage::Type differs from the actual
	 * 			one.
	 */
	NENA_EXCEPTION(ETypeMismatch);

	/**
	 * @brief	Thrown if the requested property is not set.
	 */
	NENA_EXCEPTION(EPropertyNotDefined);

protected:
	IMessageProcessor *from;
	IMessageProcessor *to;

	Type type;

	std::string className;

	boost::shared_ptr<CFlowState> flowState;

	/**
	 * @brief	Cross-"layer" properties.
	 */
	std::map<PropertyId, boost::shared_ptr<CMorphableValue> > properties;

public:

#ifdef DEBUG_MESSAGES_LOOPDETECTION
	// for debugging: detect message loops
	std::list<IMessageProcessor *> visitedProcessors;
#endif // DEBUG_MESSAGES_LOOPDETECTION
	/**
	 * @brief Constructor
	 *
	 * @param from		Sender
	 * @param to		Receiver
	 * @param type		Message type
	 */
	IMessage(IMessageProcessor* from = NULL, IMessageProcessor* to = NULL, const Type type = t_none)
	{
		this->from = from;
		this->to = to;
		this->type = type;
		className = "IMsg";
	}

	/**
	 * @brief Destructor
	 */
	virtual ~IMessage()
	{
		properties.clear();
	}

	// setters & getters

	/**
	 * @brief Return name identifying this class
	 */
	std::string getClassName() const
	{
		return className;
	}

	/**
	 * @brief	Return sender of message
	 */
	virtual IMessageProcessor *getFrom()
	{
		return from;
	}

	/**
	 * @brief	Set sender of message
	 */
	virtual void setFrom(IMessageProcessor* from)
	{
		this->from = from;
	}

	/**
	 * @brief	Return receiver of message
	 */
	virtual IMessageProcessor *getTo()
	{
		return to;
	}

	/**
	 * @brief	Set receiver of message
	 */
	virtual void setTo(IMessageProcessor* to)
	{
		this->to = to;
	}

	/**
	 * @brief	Return message type
	 */
	virtual Type getType() const
	{
		return type;
	}

	/**
	 * @brief	Set message type
	 */
	virtual void setType(const Type type)
	{
		this->type = type;
	}

	/**
	 * @brief	Set flow state
	 */
	virtual void setFlowState(boost::shared_ptr<CFlowState> flowState)
	{
		this->flowState = flowState;
	}

	/**
	 * @brief	Return flow state (may be NULL)
	 */
	virtual boost::shared_ptr<CFlowState> getFlowState()
	{
		return flowState;
	}

	/**
	 * @brief	Return a size estimate of the message.
	 *
	 * 			Currently only used for DEBUG_MESSAGES_PROFILE
	 */
	virtual std::size_t getSize() const
	{
		return 0;
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
		boost::shared_ptr<IMessage> msg = shared_from_this();
		boost::shared_ptr<T> r = boost::dynamic_pointer_cast<T, IMessage>(msg);
		return r;
	}

	/**
	 * @brief	Static cast to a subclass. Does not perform any type checking.
	 *
	 * @return	Pointer to subclass.
	 */
	template<class T>
	boost::shared_ptr<T> cast_static()
	{
		boost::shared_ptr<IMessage> msg = shared_from_this();
		boost::shared_ptr<T> r = boost::static_pointer_cast<T, IMessage>(msg);
		return r;
	}

	/**
	 * @brief	Safely set a property, erasing the old one if necessary.
	 */
	inline void setProperty(const PropertyId pid, boost::shared_ptr<CMorphableValue> val)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
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
	 * @brief	Safely set a property, erasing the old one if necessary.
	 * 			Convenience method.
	 */
	template<class T>
	inline void setProperty(const PropertyId pid, boost::shared_ptr<T> val)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
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
	 * @brief	Safely set a property, erasing the old one if necessary.
	 * 			Convenience method. Ownership of val is transfered to a shared_ptr.
	 */
	template<class T>
	inline void setProperty(const PropertyId pid, T* val)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			if (val == 0) {
				properties.erase(pit);
				return;
			}

		}

		if (val != 0)
			properties[pid] = boost::shared_ptr<CMorphableValue>(val);
	}

	/**
	 * @brief	Safely return a property.
	 *
	 * 			As template parameter, use the effective value type, e.g.
	 * 			int, string, etc. Use CMorphableValue getProperty() if you
	 * 			do not know the effective value type.
	 *
	 * @throw	EPropertyNotDefined if the property is not set.
	 * @throw	CMorphableValue::EValueTypeMismatch if the given value type
	 * 			does not match the effective value type.
	 *
	 * @return	The property value.
	 */
	template<typename T>
	boost::shared_ptr<T> getProperty(const PropertyId pid) throw (EPropertyNotDefined,
			CMorphableValue::EValueTypeMismatch)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			boost::shared_ptr<T> r = pit->second->cast<T>();
			return r;

		} else {
			throw EPropertyNotDefined();

		}
	}

	/**
	 * @brief	Safely return a property.
	 *
	 * @throw	EPropertyNotDefined if the property is not set.
	 *
	 * @return	The property's value.
	 */
	const boost::shared_ptr<CMorphableValue> getProperty(const PropertyId pid) throw (EPropertyNotDefined)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			return pit->second;

		} else {
			throw EPropertyNotDefined();

		}
	}

	/**
	 * @brief	Convenience function: Checks if the given property exists
	 * 			and returns its value in the second parameter
	 *
	 * @param	pid		Property ID
	 * @param	mv		Reference to CMorphableValue pointer variable where
	 * 					the address of the result is stored in case the
	 * 					property exists. The value is undefined if the property
	 * 					does not exist.
	 *
	 * @return	True if the requested property exists, false otherwise
	 */
	bool hasProperty(const PropertyId pid, boost::shared_ptr<CMorphableValue>& mv)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			mv = pit->second; // return a pointer
			return true;

		} else {
			return false;

		}
	}

	/**
	 * @brief	Convenience function: Checks if the given property exists
	 *
	 * @param	pid		Property ID
	 *
	 * @return	True if the requested property exists, false otherwise
	 */
	bool hasProperty(const PropertyId pid) const
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			return true;
		} else {
			return false;
		}
	}

	/**
	 * @brief 	If messages are sent in loops by purpose, this method must be
	 * 			called in order to flush the loop detection stack.
	 */
	inline void flushVisitedProcessors()
	{
#ifdef DEBUG_MESSAGES_LOOPDETECTION
		visitedProcessors.clear();
#endif // DEBUG_MESSAGES_LOOPDETECTION
	}

};

/**
 * @brief	Custom event.
 */
class IEvent : public IMessage
{
public:
	IEvent(IMessageProcessor *from = NULL, IMessageProcessor *to = NULL) :
			IMessage(from, to, IMessage::t_event)
	{
		className += "::IEvent";
	}

	virtual ~IEvent()
	{
	}

	virtual const std::string getId() const = 0;

	virtual boost::shared_ptr<IEvent> clone() const = 0;
};

/**
 * @brief Timer events.
 *
 * Single shot timer.
 */
class CTimer : public IMessage
{
public:
	double timeout;		///< Timeout in seconds

	/**
	 * @brief Constructor.
	 *
	 * @param timeout	Timeout in seconds
	 * @param proc		Node arch entity the event is linked to
	 */
	CTimer(const double timeout, IMessageProcessor* proc) :
			IMessage(proc, proc, IMessage::t_timer), // self-message
			timeout(timeout)
	{
		className += "::CTimer";
	}

	/**
	 * @brief Destructor.
	 */
	virtual ~CTimer()
	{
	}
};

/**
 * @brief Interface to a message queue
 *
 * Implementation may fit this to its needs.
 */
class IMessageQueue
{
public:
	virtual ~IMessageQueue()
	{
	}

	virtual boost::shared_ptr<IMessage> pop() = 0;
	virtual void push(boost::shared_ptr<IMessage> msg) = 0;

	virtual bool empty() const = 0;
	virtual size_t size() const = 0;

};

/**
 * @brief Interface to message scheduler.
 *
 * This may represent a thread on a real system.
 */
class IMessageScheduler
{
public:
	/**
	 * @brief	Message processor is unknown to scheduler
	 */
	NENA_EXCEPTION(EUnknowMessageProcessor);

	/**
	 * @brief	Message processor already exists
	 */
	NENA_EXCEPTION(EAlreadyRegistered);

	/**
	 * @brief	Message loop detected
	 */
	NENA_EXCEPTION(EMessageLoop);

	/**
	 * @brief	Bad call
	 *
	 * TODO: merge(r299:646): check necessity
	 */
	NENA_EXCEPTION(EBadCall);

	/**
	 * @brief	Scheduler not responsible for given message
	 *
	 * TODO: merge(r299:646): check necessity
	 */
	NENA_EXCEPTION(ENotResponsible);

private:
	std::string privateName;	///< name given by config file

protected:
	std::map<IMessageProcessor*, boost::shared_ptr<IMessageQueue> > queues;

	std::string name;

	IMessageScheduler* parent;

public:
	/**
	 * @brief Constructor.
	 */
	IMessageScheduler(IMessageScheduler* parent) :
			parent(parent)
	{
		name = "IMS";
	}
	;

	/**
	 * @brief Constructor.
	 */
	IMessageScheduler(IMessageScheduler* parent, std::string name) :
			privateName(name), parent(parent)
	{
		name = "IMS";
	}
	;

	/**
	 * @brief Destructor.
	 */
	virtual ~IMessageScheduler()
	{
		queues.clear(); // erases remaining messages
	}
	;

	/**
	 * @brief Return name identifying this class
	 */
	std::string getClassName() const
	{
		return name;
	}
	;

	/**
	 * @brief return the private name
	 */
	std::string getPrivateName() const
	{
		return privateName;
	}

	/**
	 * @brief set private name
	 */
	void setPrivateName(std::string name)
	{
		privateName = name;
	}

	/**
	 * @brief return a unique name string for this MessageScheduler
	 */
	virtual const std::string & getId() const = 0;

	/**
	 * @brief Send a message to another processing unit
	 *
	 * @param msg	Message to send
	 */
	virtual void sendMessage(boost::shared_ptr<IMessage>& msg) throw (EUnknowMessageProcessor, EMessageLoop) = 0;

	/**
	 * @brief 	Send a message to another processing unit
	 * 			Convenience method, calls sendMessage(boost::shared_ptr<IMessage>& msg)
	 *
	 * @param msg	Message to send
	 */
	template<class T>
	void sendMessage(boost::shared_ptr<T>& msg) throw (EUnknowMessageProcessor, EMessageLoop)
	{
		boost::shared_ptr<IMessage> m = boost::dynamic_pointer_cast<IMessage>(msg);
		assert(m != NULL);
		sendMessage(m);
	}

	/**
	 * @brief Set a single shot time
	 *
	 * @param timer	Timer to be set
	 */
	virtual void setTimer(boost::shared_ptr<CTimer>& timer) = 0;

	/**
	 * @brief Cancel a timer
	 *
	 * @param timer Timer to cancel
	 */
	virtual void cancelTimer(boost::shared_ptr<CTimer> & timer) = 0;

	/**
	 * @brief 	Cancel a single shot time. Convenience method. Ownership of timer is
	 * 			transfered to a shared_ptr.
	 *
	 * @param timer	Timer to be canceled
	 */
	void cancelTimer(CTimer* timer)
	{
		boost::shared_ptr<CTimer> t(timer);
		cancelTimer(t);
	}

	/**
	 * @brief 	Cancel a single shot time
	 * 			Convenience method, calls cancelTimer(boost::shared_ptr<CTimer>& timer)
	 *
	 * @param timer	Timer to be canceled
	 */
	template<class T>
	void cancelTimer(boost::shared_ptr<T> timer)
	{
		boost::shared_ptr<CTimer> t = boost::dynamic_pointer_cast<CTimer>(timer);
		assert(t != NULL);
		cancelTimer(t);
	}

	/**
	 * @brief 	Set a single shot time. Convenience method. Ownership of timer is
	 * 			transfered to a shared_ptr.
	 *
	 * @param timer	Timer to be set
	 */
	void setTimer(CTimer* timer)
	{
		boost::shared_ptr<CTimer> t(timer);
		setTimer(t);
	}

	/**
	 * @brief 	Set a single shot time
	 * 			Convenience method, calls setTimer(boost::shared_ptr<CTimer>& timer)
	 *
	 * @param timer	Timer to be set
	 */
	template<class T>
	void setTimer(boost::shared_ptr<T> timer)
	{
		boost::shared_ptr<CTimer> t = boost::dynamic_pointer_cast<CTimer>(timer);
		assert(t != NULL);
		setTimer(t);
	}

	/**
	 * @brief Process all waiting messages and call respective receivers.
	 * Returns if all queues are empty.
	 */
	virtual void processMessages() throw (EUnknowMessageProcessor, EBadCall) = 0;

	/**
	 * @brief Register a new message processor belonging to this scheduler
	 */
	virtual void registerMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor, EAlreadyRegistered) = 0;

	/**
	 * @brief Unregister a message processor belonging to this scheduler
	 */
	virtual void unregisterMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor) = 0;

	/**
	 * @brief triggers Scheduler to process messages
	 */
	virtual void run() = 0;

	/**
	 * @brief stops Scheduler
	 */
	virtual void stop() = 0;

	/**
	 * @brief Test if scheduler handles messages for a MessageProcessor
	 *
	 * @param	proc	MessageProcessor to look up
	 *
	 * @return True if this scheduler knows the MessageProcessor, false otherwise
	 */
	virtual bool hasMessageProcessor(IMessageProcessor* proc) = 0;

	/**
	 * @brief Takes over message from other message scheduler, which has no corresponding message processor
	 */
	virtual void passMessage(boost::shared_ptr<IMessage> msg) = 0;

};

/**
 * @brief Any class processing a message.
 *
 * Each message processor is connected to a scheduler which manages its incoming
 * and outgoing queues. The scheduler will also handle interprocessor
 * communications.
 */
class IMessageProcessor
{
public:

	/**
	 * @brief Message Processor failed to load config
	 */
	NENA_EXCEPTION(EConfig);

	/**
	 * @brief	Thrown if the message cannot be handled by the addressed
	 * 			message processor.
	 */
	NENA_EXCEPTION(EUnhandledMessage);

private:
	std::string componentId;		///< URI of component (optional)
	nena::hash_t componentIdHash;	///< NENA hash of URI

protected:
	IMessageScheduler *scheduler;	///< associated scheduler
	IMessageProcessor *prev; 		///< "upper", towards application
	IMessageProcessor *next; 		///< "lower", towards network

	std::string className; 			///< name identifying this class

	std::map<std::string, std::list<IMessageProcessor*> > eventListeners; ///< custom event listeners

	/**
	 * @brief	Send a message via the scheduler (convenience function).
	 */
	inline void sendMessage(boost::shared_ptr<IMessage> msg) throw (IMessageScheduler::EUnknowMessageProcessor,
			IMessageScheduler::EMessageLoop)
	{
		if (msg->getFrom() != this) {
			DBG_FAIL("Sender of message not set correctly");
		}
		scheduler->sendMessage(msg);
	}

	/**
	 * @brief	Register an event ID
	 */
	inline void registerEvent(const std::string& eventId)
	{
		eventListeners[eventId] = std::list<IMessageProcessor*>();
	}

	/**
	 * @brief	Notify external listeners for a specific custom event of
	 * 			this processor.
	 */
	void notifyListeners(const IEvent& event)
	{
		std::map<std::string, std::list<IMessageProcessor*> >::iterator mit;

		mit = eventListeners.find(event.getId());
		if (mit == eventListeners.end()) {
			DBG_WARNING(FMT("%1%: Attempt to notify unknown event %2%") % getId() % event.getId());

		} else {
//			DBG_DEBUG(FMT("%1%: Notifying on event %2%") % getClassName() % event.getId());

			std::list<IMessageProcessor*>::iterator lit;
			for (lit = mit->second.begin(); lit != mit->second.end(); lit++) {
//				DBG_DEBUG(FMT("%1%: Notifying %2% on event %3%") % getClassName() % (*lit)->getClassName() % event.getId());
				boost::shared_ptr<IEvent> e = event.clone();
				e->setFrom(this);
				e->setTo(*lit);
				sendMessage(e);
			}
		}
	}

	/**
	 * @brief	Set component URI.
	 */
	void setId(const std::string& uri)
	{
		componentId = uri;
		if (componentId.empty()) {
			componentIdHash = 0;

		} else {
			componentIdHash = nena::hash_string(componentId);

		}
	}

public:
	/**
	 * @brief Constructor.
	 *
	 * @param sched		Scheduler that manages the queues of this message processor
	 */
	IMessageProcessor(IMessageScheduler *sched)
	{
		className = "IMP";
		prev = next = NULL;
		componentIdHash = 0;
		scheduler = sched;

		if (scheduler)
			scheduler->registerMessageProcessor(this);
	}

	/**
	 * @brief Destructor.
	 */
	virtual ~IMessageProcessor()
	{
		if (scheduler)
			scheduler->unregisterMessageProcessor(this);
		DBG_INFO(FMT("%1% destroyed") % getId());
	}

	/**
	 * @brief Return name identifying this class
	 */
	inline const std::string& getClassName() const
	{
		return className;
	}

	/**
	 * @brief	Return component URI. This might return an empty string.
	 */
	inline const std::string& getId() const
	{
		if (componentId.empty())
			return className;
		return componentId;
	}

	/**
	 * @brief	Return component URI hash. This might return 0 if component has
	 * 			no URI.
	 */
	inline nena::hash_t getIdHash() const
	{
		return componentIdHash;
	}

	/**
	 * @brief 	Process a message directed to this message processing unit.
	 * 			This method calls the respective process* functions depending
	 * 			on the message's type.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processMessage(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
	{
		//DBG_DEBUG(FMT("%1%: processMessage.") % className);

		assert(msg != NULL);

		switch (msg->getType()) {
		case IMessage::t_event:
			processEvent(msg);
			break;
		case IMessage::t_timer:
			processTimer(msg);
			break;
		case IMessage::t_outgoing:
			processOutgoing(msg);
			break;
		case IMessage::t_incoming:
			processIncoming(msg);
			break;
		default:
			throw EUnhandledMessage((FMT("Unknown message type (%1%)!") % msg->getType()).str());
		}
	}

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) = 0;

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) = 0;

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) = 0;

	/**
	 * @brief Process an incoming message directed towards the application.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) = 0;

	/**
	 * @brief Return message scheduler of this message processor.
	 */
	virtual IMessageScheduler *getMessageScheduler() const
	{
		return scheduler;
	}

	/**
	 * @brief Set message scheduler of this message processor.
	 */
	virtual void setMessageScheduler(IMessageScheduler *sched)
	{
		if (scheduler)
			scheduler->unregisterMessageProcessor(this);

		scheduler = sched;

		if (scheduler)
			scheduler->registerMessageProcessor(this);
	}

	/**
	 * @brief 	Return message processor that is called prior to this one
	 * 			(from application's point of view).
	 */
	virtual IMessageProcessor *getPrev() const
	{
		return prev;
	}

	/**
	 * @brief 	Set message processor that should be called prior to this one
	 * 			(from application's point of view).
	 */
	virtual void setPrev(IMessageProcessor *processor)
	{
		prev = processor;
	}

	/**
	 * @brief 	Return message processor that is called next to this one
	 * 			(from application's point of view).
	 */
	virtual IMessageProcessor *getNext() const
	{
		return next;
	}

	/**
	 * @brief 	Set message processor that should be called next to this one
	 * 			(from application's point of view).
	 */
	virtual void setNext(IMessageProcessor *processor)
	{
		next = processor;
	}

	/**
	 * @brief returns true if the MessageProcessor is thread safe
	 *
	 * TODO: merge(r299:646): check necessity
	 */
	virtual bool isThreadsafe() const
	{
		return false;
	}

	// event provider methods

	/**
	 * @brief	Register an external listener for a specific custom event of
	 * 			this processor.
	 */
	void registerListener(const std::string& eventId, IMessageProcessor* msgProc)
	{
		std::map<std::string, std::list<IMessageProcessor*> >::iterator mit;

		mit = eventListeners.find(eventId);
		if (mit == eventListeners.end()) {
			DBG_WARNING(
					FMT("%1%: Attempt to register listener (%2%) for an unknown event (%3%)") % getId() % msgProc->getId() % eventId);

		} else {
			DBG_INFO(
					FMT("%1%: Registering listener %2% for event %3%") % getId() % msgProc->getId() % eventId);
			mit->second.push_back(msgProc);

		}
	}

	/**
	 * @brief	Unregister an external listener for a specific custom event of
	 * 			this processor.
	 */
	void unregisterListener(const std::string& eventId, IMessageProcessor* msgProc)
	{
		std::map<std::string, std::list<IMessageProcessor*> >::iterator mit;

		mit = eventListeners.find(eventId);
		if (mit == eventListeners.end()) {
			DBG_WARNING(
					FMT("%1%: Attempt to unregister listener %2% for unknown event %3%") % getId() % msgProc->getId() % eventId);

		} else {
			bool success = false;
			std::list<IMessageProcessor*>::iterator lit;
			for (lit = mit->second.begin(); lit != mit->second.end(); lit++) {
				if (*lit == msgProc) {
					DBG_INFO(
							FMT("%1%: Unregistering listener %2% for event %3%") % getId() % msgProc->getId() % eventId);
					eventListeners[eventId].erase(lit);
					success = true;
					break;
				}
			}

			if (!success) {
				DBG_WARNING(
						FMT("%1%: Attempt to unregister unregistered listener %2% (event %3%)") % getId() % msgProc->getId() % eventId);
			}
		}
	}
};

#endif /* MESSAGES_H_ */
