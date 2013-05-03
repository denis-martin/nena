/**
 * simpleNetlet.cpp
 *
 * @brief Simple example Netlet
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#include "simpleNetlet.h"

#include "nena.h"
#include "netAdaptBroker.h"
#include "messageBuffer.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

using namespace std;

/**
 * Initializer for shared library. Registers factory function.
 */
static CSimpleNetletMetaData simpleNetletMetaData;

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleNetlet::CSimpleNetlet(CNena *nena, IMessageScheduler *sched) :
	INetlet(nena, sched)
{
	className += "::CSimpleNetlet";
	setPrev((IMessageProcessor*) nena->getNetletSelector());
	DBG_DEBUG(FMT("SimpleNetlet: Instantiating %1% for %2%") % SIMPLE_NETLET_NAME % SIMPLE_ARCH_NAME);

	netletSelector = static_cast<CNetletSelector*>(nena->lookupInternalService("internalservice://nena/netletSelector"));
	assert(netletSelector != NULL);
}

/**
 * Destructor
 */
CSimpleNetlet::~CSimpleNetlet()
{
	DBG_DEBUG("SimpleNetlet: ~CSimpleNetlet()");
}


/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleNetlet::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleNetlet::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CSimpleNetlet::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

//	DBG_DEBUG(FMT("SimpleNetlet: Received outgoing packet with %1% bytes") % pkt->size());

	// TODO: obsolete
	SimpleNetlet_Header header;
	header.serviceHash = nena::hash_string(pkt->getProperty<CStringValue>(IMessage::p_serviceId)->value());

	// make sure, that our counter part at the other side gets this message
	pkt->setProperty(IMessage::p_netletId, new CStringValue(getMetaData()->getId()));

	pkt->push_header(header);
	pkt->setFrom(this);
	pkt->setTo(next);
	sendMessage(pkt);
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CSimpleNetlet::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	// hand to application
	SimpleNetlet_Header header;
	pkt->pop_header(header);

//	DBG_DEBUG(FMT("SimpleNetlet: Received incoming packet with %1% bytes") % pkt->size());

	// TODO: maybe not the right place here...
	assert(pkt->getFlowState() != NULL);

//	if (pkt->getFlowState() == NULL) {
//		std::string serviceId;
//		list<std::string> services;
//		netletSelector->getRegisteredAppServices(services);
//		list<std::string>::const_iterator it;
//		for (it = services.begin(); it != services.end(); it++)
//		{
//			if (hash(*it) == header.serviceHash) {
//				serviceId = *it;
//				pkt->setProperty(IMessage::p_serviceId, new CStringValue(serviceId));
//				break;
//			}
//		}
//
//		if (serviceId.empty())
//			throw EUnhandledMessage("Service hash could not be resolved");
//
//		CFlowState::FlowId flowId = netletSelector->lookupAppService(serviceId);
//		if (flowId == 0)
//			throw EUnhandledMessage("Could not find a local flow ID");
//		pkt->setFlowState(netletSelector->lookupFlowId(flowId)->getFlowState());
//
//	}

	pkt->setFrom(this);
	pkt->setTo(prev);
	sendMessage(pkt);
}

/**
 * @brief	Returns the Netlet's meta data
 */
INetletMetaData* CSimpleNetlet::getMetaData() const
{
	return (INetletMetaData*) &simpleNetletMetaData;
}

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleNetletMetaData::CSimpleNetletMetaData()
{
	MultiplexerFactories::iterator nit= multiplexerFactories.find(getArchName());

	if (nit == multiplexerFactories.end())
		DBG_ERROR(FMT("SimpleNetlet: %1% not found => %2% cannot be loaded") % getArchName() % getId());
	else
		netletFactories[getArchName()][getId()] = (INetletMetaData*) this;
}

/**
 * Destructor
 */
CSimpleNetletMetaData::~CSimpleNetletMetaData()
{
	NetletFactories::iterator ait = netletFactories.find(getArchName());
	if (ait != netletFactories.end()) {
		map<string, INetletMetaData*>::iterator nit= ait->second.find(getId());
		if (nit != ait->second.end()) {
			ait->second.erase(nit);
		}
	}

}

/**
 * Return Netlet name
 */
std::string CSimpleNetletMetaData::getArchName() const
{
	return SIMPLE_ARCH_NAME;
}

/**
 * Return Netlet name
 */
const std::string& CSimpleNetletMetaData::getId() const
{
	return SIMPLE_NETLET_NAME;
}

/**
 * @brief	Returns true if the Netlet does not offer any transport service
 * 			for applications, false otherwise.
 */
bool CSimpleNetletMetaData::isControlNetlet() const
{
	return false;
}

/**
 * @brief	Returns confidence value for the given name (uri) and the given
 * 			requirements (req). A value of 0 means, that the Netlet cannot
 * 			handle the request at all.
 */
int CSimpleNetletMetaData::canHandle(const std::string& uri, std::string& req) const
{
	if ((uri.compare(0, 7, "node://") == 0) or
		(uri.compare(0, 6, "app://") == 0))
	{
		return 1;
	}
	return 0;
}

/**
 * Create an instance of the Netlet
 */
INetlet* CSimpleNetletMetaData::createNetlet(CNena *nodeA, IMessageScheduler *sched)
{
	return new CSimpleNetlet(nodeA, sched);
}

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
