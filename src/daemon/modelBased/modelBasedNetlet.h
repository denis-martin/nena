/** @file
 * modelBasedNetlet.h
 *
 * @brief Model-based Netlet
 *
 * (c) 2008-2011 Institut fuer Telematik, KIT, Karlsruhe, Germany
 *
 *  Created on: 10 Aug 2011
 *      Author: denis
 */

#ifndef MODELBASEDNETLET_H_
#define MODELBASEDNETLET_H_

#include "composableNetlet.h"

#include <list>

class CModelBasedNetlet : public IComposableNetlet
{
protected:
	std::list<IBuildingBlock*> bbs;

public:
	CModelBasedNetlet(CNena *nena, IMessageScheduler *sched);
	virtual ~CModelBasedNetlet();

	// from IMessageProcessor

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

	// from INetlet

	/**
	 * @brief	Returns the Netlet's meta data
	 */
	virtual INetletMetaData* getMetaData();

};

#endif /* MODELBASEDNETLET_H_ */
