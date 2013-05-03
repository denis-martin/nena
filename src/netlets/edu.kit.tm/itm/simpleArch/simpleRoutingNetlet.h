/** @file
 * simpleRoutingNetlet.h
 *
 * @brief	Simple routing Netlet
 *
 * The Simple Architecture has the following features:
 * - A simple transport Netlet. At the moment, it just hands the application's
 *   data to the multiplexer.
 * - A simple routing Netlet. A very dumb one, just creating a forwarding
 *   information base (FIB).
 * - A forwarding mechanism based on the FIB. This mechanism is realized within
 *   the multiplexer. Note, that other architectures may do the forwarding in
 *   a Netlet.
 *
 * The routing uses the node names as identifier and locator. Thus, we're
 * basically doing the same mistake all over again in this dumb demo
 * architecture.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: May 26, 2009
 *      Author: denis
 */

#ifndef SIMPLEROUTINGNETLET_H_
#define SIMPLEROUTINGNETLET_H_

#include "netlet.h"
#include "messages.h"
#include "netAdapt.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

const std::string& SIMPLE_ROUTING_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/SimpleRoutingNetlet";

class CSimpleMultiplexer;
class CSimpleRoutingNetletMetaData;

/**
 * @brief Simple neighbor discovery protocol
 */
class CSimpleRoutingNetlet : public INetlet
{
private:
	CSimpleRoutingNetletMetaData* metaData;
	CSimpleMultiplexer* multiplexer;

public:
	CSimpleRoutingNetlet(CSimpleRoutingNetletMetaData* metaData, CNena *nena, IMessageScheduler *sched);
	virtual ~CSimpleRoutingNetlet();

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
	virtual INetletMetaData* getMetaData() const;

	virtual const std::string & getId () const { return getMetaData()->getId (); }

	// own methods

	void handleTimeout();

};

/**
 * @brief Simple Routing Netlet meta data
 *
 * The Netlet meta data class has two purposes: On one hand, it describes
 * the Netlet's properties, capabilities, etc., on the other hand, it provides
 * a factory functions that will be used by the node architecture daemon to
 * instantiate new Netlets of this type.
 */
class CSimpleRoutingNetletMetaData : public INetletMetaData
{
private:
	std::string archName;
	std::string netletId;

public:
	CSimpleRoutingNetletMetaData(const std::string& archName);
	virtual ~CSimpleRoutingNetletMetaData();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;

	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched); // factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLEROUTINGNETLET_H_ */
