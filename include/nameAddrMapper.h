/** @file
 *
 * nameAddrMapper.h
 *
 * @brief 	Interface for a name/address mapper.
 *
 *			There should be at least one implementation per architecture.
 * 			Basic assumption: The name is a string and globally unique (e.g.,
 * 			an URL). The address format is completely undefined and
 * 			architecture independent.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 3, 2009
 *      Author: denis
 */

#ifndef NAMEADDRMAPPER_H_
#define NAMEADDRMAPPER_H_

#include "netlet.h"

#include <string>
#include <list>

class CNena;
class IAppConnector;

class INameAddrMapper : public IMessageProcessor
{
protected:
	CNena* nodeArch;

public:
	INameAddrMapper(CNena* nodeA, IMessageScheduler *sched) :
		IMessageProcessor(sched), nodeArch(nodeA)
	{
	}

	virtual ~INameAddrMapper()
	{
	}

	/**
	 * @brief	Adds meta-data classes to the supplied list, providing
	 * 			information about Netlets that are able to communicate
	 * 			with the given name.
	 *
	 * @param	appc		AppConnector object of the application.
	 * @param	netletList	List to which the Netlets' meta data is to be
	 * 						added.
	 */
	virtual void getPotentialNetlets(IAppConnector* appc, std::list<INetletMetaData*>& netletList) const = 0;

	// from IMessageProcessor

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
	{
		throw EUnhandledMessage("Unhandled event message!");
	};

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
	{
		throw EUnhandledMessage("Unhandled timer message!");
	}

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
	{
		throw EUnhandledMessage("Unhandled outgoing message!");
	}

	/**
	 * @brief Process an incoming message directed towards the application.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
	{
		throw EUnhandledMessage("Unhandled incoming message!");
	}
};

#endif /* NAMEADDRMAPPER_H_ */
