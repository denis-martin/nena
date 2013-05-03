/** @file
 * simpleComposedNetlet.h
 *
 * @brief Simple composed Netlet for Simple Architecture
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 14, 2009
 *      Author: denis
 */

#ifndef SIMPLECOMPOSEDNETLET_H_
#define SIMPLECOMPOSEDNETLET_H_

#include "composableNetlet.h"
#include "netletSelector.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

/**
 * @brief	A Netlet taking its actual building block configuration from
 * 			a generated source file.
 *
 * 			This is a base class for composed Netlets within the Simple
 * 			Architecture.
 *
 * @TODO	Per architecture, a single template for a composable Netlet would
 * 			be sufficient.
 */
class CSimpleComposedNetlet : public IComposableNetlet
{
protected:
	IBuildingBlock* event_bb; // TODO: define CSimpleComposedNetlet events
	CNetletSelector* netletSelector;

public:
	CSimpleComposedNetlet(CNena *nodeA, IMessageScheduler *sched);
	virtual ~CSimpleComposedNetlet();

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
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLECOMPOSEDNETLET_H_ */
