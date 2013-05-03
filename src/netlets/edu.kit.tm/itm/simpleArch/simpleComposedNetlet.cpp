/**@file
 * simpleComposedNetlet.cpp
 *
 * @brief	Simple composed Netlet for Simple Architecture
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 14, 2009
 *      Author: denis
 */

#include "simpleComposedNetlet.h"

#include "nena.h"
#include "netAdaptBroker.h"
#include "messageBuffer.h"

#include <boost/shared_ptr.hpp>

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

using namespace std;
using boost::shared_ptr;

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleComposedNetlet::CSimpleComposedNetlet(CNena *nena, IMessageScheduler *sched) :
	IComposableNetlet(nena, sched), event_bb(NULL)
{
	className += "::CSimpleComposedNetlet";

	netletSelector = static_cast<CNetletSelector*>(nena->lookupInternalService("internalservice://nena/netletSelector"));
	assert(netletSelector != NULL);

	setPrev((IMessageProcessor*) netletSelector);
}

/**
 * Destructor
 */
CSimpleComposedNetlet::~CSimpleComposedNetlet()
{
}


/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleComposedNetlet::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	if (event_bb != NULL) {
		msg->setFrom(this);
		msg->setTo(event_bb);
		sendMessage(msg);

	} else {
		DBG_ERROR("Unhandled event!");
		throw EUnhandledMessage();

	}
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleComposedNetlet::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CSimpleComposedNetlet::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	// some sanity checks
	assert(config != NULL);
	if (config->outgoingChain.size() == 0)
		throw EUnhandledMessage("Outgoing chain is empty!");

	shared_ptr<IBuildingBlock> bb = getBuildingBlockByName(config->outgoingChain.front());
	if (!bb)
		throw EUnhandledMessage((FMT("Error in BB configuration: %1% not found") %
			config->outgoingChain.front()).str());

	// check whether the packet already traveled through one of our building blocks
	if (pkt->getFrom() == getBuildingBlockByName(config->outgoingChain.back()).get()) {
		// yes, so hand it off towards the network

		// make sure, that our counter part at the other side gets this message
		if (!pkt->hasProperty(IMessage::p_netletId))
			pkt->setProperty(IMessage::p_netletId, new CStringValue(getMetaData()->getId()));
		pkt->setProperty(IMessage::p_srcId, new CStringValue(nena->getNodeName()));

		pkt->setFrom(this);
		pkt->setTo(next);
		sendMessage(pkt);

	} else {
		// nope, so reset its loop trace and give it to the first BB
		pkt->flushVisitedProcessors();
		pkt->setFrom(this);
		pkt->setTo(bb.get());
		sendMessage(pkt);

	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CSimpleComposedNetlet::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	shared_ptr<IBuildingBlock> bb = getBuildingBlockByName(config->incomingChain.back());
	assert(bb);

	// check whether the packet already traveled through our building blocks
	if (pkt->getFrom() == bb.get()) {
		// yes
		if (getMetaData()->isControlNetlet())
			throw EUnhandledMessage("Attempting to send a message towards an application, but we are a control Netlet!");

		pkt->setFrom(this);
		pkt->setTo(prev);
		sendMessage(pkt);

	} else {
		// nope, so reset its loop trace and give it to the first BB

		bb = getBuildingBlockByName(config->incomingChain.front());
		assert(bb);

//		DBG_INFO("Handing packet to first BB");
		pkt->flushVisitedProcessors();
		pkt->setFrom(this);
		pkt->setTo(bb.get());
		sendMessage(pkt);

	}

}

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
