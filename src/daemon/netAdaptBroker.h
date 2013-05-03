/** @file
 * netAdaptBroker.h
 *
 * @brief Network Access Broker (Manager)
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#ifndef NETADAPTBROKER_H_
#define NETADAPTBROKER_H_

#include "messages.h"

#include "netAdapt.h"

#include <map>
#include <list>
#include <string>

class CNena;

/**
 * @brief Network Adaptor Broker
 *
 * TODO: Maybe this should not be an IMessageProcessor. Instead, the Multiplexers
 * should connect directly to the network accesses.
 */
class CNetAdaptBroker : public IMessageProcessor
{
protected:
	CNena* na;

	std::map<std::string, std::list<INetAdapt *> > netAdapts; ///< List of adaptors per architecture

public:
	CNetAdaptBroker(CNena* nena, IMessageScheduler *sched);
	virtual ~CNetAdaptBroker();

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

	void registerNetAdapt(INetAdapt *netAdapt);

	std::list<INetAdapt *>& getNetAdapts(std::string archName);
	INetAdapt * getNetAdapt(std::string netadaptName);

};

#endif /* NETADAPTBROKER_H_ */
