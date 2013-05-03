/**
 * simpleMultiplexer.cpp
 *
 * @brief Multiplexer for "Simple Architecture"
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#include "ipv6Multiplexer.h"

#include "debug.h"
#include "messageBuffer.h"
#include "nena.h"
#include "netletSelector.h"
#include "management.h"
#include "localRepository.h"

#include "md5/md5.h"

#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/split.hpp"

#include <string>
#include <set>
#include <algorithm>

using namespace std;
using boost::shared_ptr;

namespace ipv6 {

/**
 * @brief	Constructor
 */
CIPv6NameAddrMapper::CIPv6NameAddrMapper(const std::string& archName, CNena* nodeA, IMessageScheduler *sched) :
	INameAddrMapper(nodeA, sched), archName(archName)
{
	string id = archName;
	id = id.replace(0, id.find(':'), "addrMapper");
	setId(id);
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CIPv6NameAddrMapper::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> ev = msg->cast<IEvent>();

	DBG_DEBUG(FMT("Received unknown event message %1% from %2%")
		% ev->getId() % ev->getFrom()->getClassName());
	throw EUnhandledMessage("Unhandled event message");
}

/**
 * @brief	Destructor
 */
CIPv6NameAddrMapper::~CIPv6NameAddrMapper()
{
}

/**
 * @brief	Adds meta-data classes to the supplied list, providing
 * 			information about Netlets that are able to communicate
 * 			with the given name.
 *
 * 			Currently, we assume that every non-control Netlet can
 * 			handle all given names.
 *
 * @param	appc		AppConnector object of the application.
 * @param	netletList	List to which the Netlets' meta data is to be
 * 						added.
 */
void CIPv6NameAddrMapper::getPotentialNetlets(IAppConnector* appc, std::list<INetletMetaData*>& netletList) const
{
	if (appc->getRemoteURI().empty())
		return;

	list<INetlet*> netlets;
	nodeArch->getNetlets(archName, netlets);

	std::string req = std::string(appc->getRequirements()); // get requirements from app connector

	list<INetlet*>::const_iterator it;
	for (it = netlets.begin(); it != netlets.end(); it++)
	{
		if (!(*it)->getMetaData()->isControlNetlet()) {
			if ((*it)->getMetaData()->canHandle(appc->getRemoteURI(), req) > 0) {
				netletList.push_back((*it)->getMetaData());
			}
		}
	}
}

/* ========================================================================= */

/**
 * Constructor
 */
CIPv6Multiplexer::CIPv6Multiplexer(IMultiplexerMetaData *metaData, CNena *nodeA, IMessageScheduler *sched)
	:INetletMultiplexer(metaData, nodeA, sched)
{
	className += "::CIPv6Multiplexer";

	string id = metaData->getArchName();
	id = id.replace(0, id.find(':'), "multiplexer");
	setId(id);

	nameAddrMapper = new CIPv6NameAddrMapper(metaData->getArchName(), nodeArch, sched);

	netletSelector = static_cast<CNetletSelector*>(nodeA->lookupInternalService("internalservice://nena/netletSelector"));
	assert(netletSelector != NULL);

	netletSelector->registerNameAddrMapper(nameAddrMapper);

	registerEvent(EVENT_IPV6_UNHANDLEDPACKET);
}

/**
 * Constructor
 */
CIPv6Multiplexer::~CIPv6Multiplexer()
{
	// TODO: unregister name/addr mapper?
	delete nameAddrMapper;
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CIPv6Multiplexer::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<Management::Event_UpdatedConfig> ev = msg->cast<Management::Event_UpdatedConfig>();
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CIPv6Multiplexer::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CIPv6Multiplexer::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	shared_ptr<CMessageBuffer> mbuf = msg->cast<CMessageBuffer>();
	assert(mbuf != NULL);

	// consume end-of-stream marker if it arrives here
	bool endOfStream = false;
	try {
		endOfStream = msg->getProperty<CBoolValue>(IMessage::p_endOfStream)->value();
	} catch (exception& e) {
		// nothing
	}
	if (endOfStream) {
		assert(mbuf->size() == 0);
		mbuf->getFlowState()->decOutFloatingPackets();
		DBG_INFO(FMT("%1%: end-of-stream marker consumed") % getId());
		return;
	}

	throw EUnhandledMessage("NYI");
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CIPv6Multiplexer::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	shared_ptr<CMessageBuffer> mbuf = msg->cast<CMessageBuffer>();
	if (mbuf == NULL) throw EUnhandledMessage("Incoming message not of type CMessageBuffer.");
	
	IPv6_Header hdr;
	mbuf->pop_header(hdr);

	DBG_DEBUG(FMT("%1%: v %2% tc %3% fl %4% pl %5% nh %6% hl %7%") % getId() % (int) hdr.getVersion() % (int) hdr.getTrafficClass() % hdr.getFlowLabel()
		% hdr.getPayloadLength() % (int) hdr.getNextHeader() % (int) hdr.getHopLimit());

	string proto;

	switch (hdr.getNextHeader())
	{
	case IPPROTO_UDP: proto = "udp"; break;
	case IPPROTO_TCP: proto = "tcp"; break;
	default:
		proto = (FMT("%1%") % (int) hdr.getNextHeader()).str();
		break;
	}

	DBG_DEBUG(FMT("%1%: src %2%, dst %3% (%4%)") % getId() % hdr.getSrcAddrStr() % hdr.getDstAddrStr() % proto);

	//throw EUnhandledMessage();
}

/**
 * @brief	Called if new Netlets of this architecture were added to the system.
 */
void CIPv6Multiplexer::refreshNetlets()
{
	hashedNetlets.clear();

	list<INetlet*> netlets;
	nodeArch->getNetlets(getMetaData()->getArchName(), netlets);

	list<INetlet*>::const_iterator it;
	for (it = netlets.begin(); it != netlets.end(); it++) {
		hashedNetlets[nena::hash_string((*it)->getMetaData()->getId())] = *it;
		DBG_DEBUG(FMT("%1%: Found %2% hash: %3$08x") % getId() % (*it)->getMetaData()->getId() % nena::hash_string((*it)->getMetaData()->getId()));
		(*it)->setNext(this);
	}
}

string CIPv6Multiplexer::getStats()
{
	return string();
}

/* ========================================================================= */

/**
 * Constructor
 */
CIPv6MultiplexerMetaData::CIPv6MultiplexerMetaData()
	: IMultiplexerMetaData(), archName(IPV6_ARCH_NAME)
{
	MultiplexerFactories::iterator mit = multiplexerFactories.find(getArchName());

	if (mit != multiplexerFactories.end())
		DBG_ERROR(FMT("SimpleArch: FIXME: %1% already loaded! Skipping factory registration...") % getArchName());
	else
		multiplexerFactories[getArchName()] = (IMultiplexerMetaData*) this;
}

/**
 * Destructor
 */
CIPv6MultiplexerMetaData::~CIPv6MultiplexerMetaData()
{
	MultiplexerFactories::iterator mit = multiplexerFactories.find(getArchName());

	if (mit != multiplexerFactories.end())
		multiplexerFactories.erase(getArchName());
}

/**
 * Return Netlet name
 */
std::string CIPv6MultiplexerMetaData::getArchName() const
{
	return archName;
}

/**
 * Create an instance of the Netlet
 */
INetletMultiplexer* CIPv6MultiplexerMetaData::createMultiplexer(CNena *nodeA, IMessageScheduler *sched)
{
	return new CIPv6Multiplexer((IMultiplexerMetaData *) this, nodeA, sched);
}

/* ========================================================================= */

/**
 * Initializer for shared library.
 */
static CIPv6MultiplexerMetaData ipv6MultiplexerMetaData;

} // namespace ipv6
