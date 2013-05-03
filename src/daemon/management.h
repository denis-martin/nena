/** @file
 * management.h
 *
 * @brief "Architect" of the Node
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 12, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _MANAGEMENT_H_
#define _MANAGEMENT_H_

#include <boost/scoped_ptr.hpp>
#include <exceptions.h>
#include <systemWrapper.h>

#include <string>

#include "nena.h"
#include "messages.h"
#include "netletRepository.h"
#include "netAdaptBroker.h"
#include "netletSelector.h"

#define EVENT_SIMPLEARCH_UPDATEDCONFIG	"event://daemon/UpdatedConfig"

class Management : public IMessageProcessor
{
public:
	/// Config file not found
	NENA_EXCEPTION(EFile);

	/// Configuration error
	NENA_EXCEPTION(EConfig);

	/**
	 * @brief	Event emitted if configuration has changed
	 *
	 */
	class Event_UpdatedConfig : public IEvent
	{
	public:
		Event_UpdatedConfig(IMessageProcessor *from = NULL, IMessageProcessor *to = NULL) :
				IEvent(from, to)
		{
			className += "::Event_UpdatedConfig";
		}

		virtual ~Event_UpdatedConfig()
		{
		}

		virtual const std::string getId() const
		{
			return EVENT_SIMPLEARCH_UPDATEDCONFIG;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(new Event_UpdatedConfig(from, to));
		}
	};

public:
	Management(ISystemWrapper * s, CNena * na);
	virtual ~Management();

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
	 * @brief will assemble the nodearch based on config file
	 */
	void assemble();

	/**
	 * @brief start node
	 */
	void run();

	/**
	 * @brief stop node
	 */
	void stop();

	/**
	 * @brief dissassemble node and shut down MessageProcessors in a controlled manner
	 */
	void disassemble();

	/**
	 * @brief return pointer to system wrapper
	 */
	ISystemWrapper * getSys() const;

	/**
	 * @brief Return the NA Broker
	 */
	CNetAdaptBroker * getNetAdaptBroker() const;

	/**
	 * @brief Return the Netlet selector
	 */
	CNetletSelector * getNetletSelector() const;

	/**
	 * @brief Return the repository
	 */
	INetletRepository * getRepository() const;

	/**
	 * @brief process xml config message
	 *
	 * @param msg xml message
	 *
	 * @return xml answer string
	 */
	virtual std::string processCommand(std::string msg);

private:
	class Private;
	boost::scoped_ptr<Private> d;

	const Private * d_func() const;
	Private * d_func();
};

#endif
