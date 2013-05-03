/*
 * simpleRoutingNetlet.cpp
 *
 *  Created on: May 26, 2009
 *      Author: denis
 */

// architecture specific includes
#include "simpleRoutingNetlet.h"
#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

// general includes
#include "messageBuffer.h"

// daemon
#include "nena.h"
#include "netAdaptBroker.h"
#include "localRepository.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

using namespace std;
using boost::shared_ptr;

/* ========================================================================= */

/**
 * @brief	Timeout to send route information.
 *
 * Single shot timer.
 */
class CSimpleRoutingNetlet_Timeout: public CTimer
{
public:
	/**
	 * @brief Constructor.
	 *
	 * @param timeout	Timeout in seconds
	 * @param proc		Node arch entity the event is linked to (default = NULL)
	 */
	CSimpleRoutingNetlet_Timeout(double timeout, IMessageProcessor *proc) :
		CTimer(timeout, proc)
	{
//		DBG_INFO(FMT("SimpleRoutingNetlet_Timeout: created @ %1%") % this);
	}

	/**
	 * @brief Destructor.
	 */
	virtual ~CSimpleRoutingNetlet_Timeout()
	{
//		DBG_INFO(FMT("SimpleRoutingNetlet_Timeout: dying with address %1%") % this);
	}

};

/* ========================================================================= */

/**
 * @brief	Header class for routing information exchange (RIX) message.
 * 			This packet is sent blindly over an interface and contains locators
 * 			that we (the sender) can reach via *another* interface.
 *
 * 			The header format is as follows:
 * 			[A][BBBB][CC][D][BBBB][CC][D]...
 * 			Whereas
 * 			A is the number of list entries (one byte)
 * 			B is the IPv4 address (in network format)
 * 			C is the IP port (in network format)
 *          D is the number of hops (max 255)
 */
class CSimpleRoutingNetlet_Header_RIX: public IHeader
{
public:
	class ForwardingInfo
	{
	public:
		ipv4::CLocatorValue loc;
		int hops;

		ForwardingInfo(ipv4::CLocatorValue loc, int hops) : loc(loc), hops(hops) {};
	};

	list<ForwardingInfo> nodes; // list of nodes that can be reached via us and their hops

	/**
	 * @brief	Serialize all relevant data into a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
//		DBG_INFO(FMT("CSimpleRoutingNetlet_Header_RIX serialize:"));
		
		size_t s = sizeof(unsigned char) +
			(sizeof(uint32_t) + sizeof(ushort) + sizeof(unsigned char)) *
				nodes.size();
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(s));

		buffer->push_uchar((unsigned char) nodes.size());
		
//		DBG_INFO(FMT("nodes.size(): %1$u") % nodes.size());

		list<ForwardingInfo>::const_iterator it;
		for (it = nodes.begin(); it != nodes.end(); it++) {
			assert(it->loc.isValid()); // TODO: to be handled more gracefully
			buffer->push_ulong(it->loc.getAddr());
			buffer->push_ushort(it->loc.getPort());
			buffer->push_uchar(it->hops);
//			DBG_INFO(FMT("it->loc.getAddr(): %1%, it->loc.getPort(): %2%, it->hops: %3%") % it->loc.getAddr() % it->loc.getPort() % it->hops);
		}

		return buffer;
	}

	/**
	 * @brief 	De-serialize all relevant data from a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		assert(buffer != NULL);

//		DBG_INFO(FMT("CSimpleRoutingNetlet_Header_RIX deserialize:"));
		
		unsigned char listLength = buffer->pop_uchar();
		
//		DBG_INFO(FMT("listLength: %1$u") % listLength);
		
		for (unsigned char i = 0; i < listLength; i++) {
			ipv4::CLocatorValue loc;
			int hops;
			loc.setAddr(buffer->pop_ulong());
			loc.setPort(buffer->pop_ushort());
			hops = buffer->pop_uchar();
			
//			DBG_INFO(FMT("loc.getAddr(): %1%, loc.getPort(): %2%, hops: %3%") % loc.getAddr() % loc.getPort() % hops);
			
			nodes.push_back(ForwardingInfo(loc, hops));
		}

	}

};

/* ========================================================================= */

CSimpleRoutingNetlet::CSimpleRoutingNetlet(CSimpleRoutingNetletMetaData* metaData, CNena *nena, IMessageScheduler *sched)
	: INetlet(nena, sched), metaData(metaData)
{
	className += "::CSimpleRoutingNetlet";
	setId(metaData->getId());
	multiplexer = (CSimpleMultiplexer*) nena->getMultiplexer(getMetaData()->getArchName());
	assert(multiplexer != NULL);

	DBG_DEBUG(FMT("%1% instantiated for %2%") % getId() % getMetaData()->getArchName());

	assert(scheduler != NULL);
	scheduler->setTimer(new CSimpleRoutingNetlet_Timeout(1.0 + nena->getSys()->random(), this));
}

CSimpleRoutingNetlet::~CSimpleRoutingNetlet() {

}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleRoutingNetlet::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleRoutingNetlet::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CSimpleRoutingNetlet_Timeout> timer = msg->cast<CSimpleRoutingNetlet_Timeout>();
	if (timer.get()) {
//		DBG_DEBUG(FMT("CSimpleRoutingNetlet got valid timer %1%") % timer.get());
		handleTimeout();

	} else {
		DBG_ERROR("Unhandled timer message!");
		throw EUnhandledMessage();

	}
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CSimpleRoutingNetlet::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled outgoing message!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CSimpleRoutingNetlet::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	INetAdapt* na = pkt->getProperty<CPointerValue<INetAdapt> >(IMessage::p_netAdapt)->value();
	ipv4::CLocatorValue nextHopLoc = *pkt->getProperty<ipv4::CLocatorValue>(IMessage::p_srcLoc);

	SimpleFib& fib = multiplexer->getFib();
	CSimpleRoutingNetlet_Header_RIX rix;
	pkt->pop_header(rix);

	DBG_INFO(FMT("Incoming RIX from %1%") % nextHopLoc.toStr());

	list<ipv4::CLocatorValue> localLocList;

	// slightly inefficient to do it every time...
	list<INetAdapt*>& netAdapts(nena->getNetAdaptBroker()->getNetAdapts(getMetaData()->getArchName()));
	list<INetAdapt*>::iterator nas_it;
	for (nas_it = netAdapts.begin(); nas_it != netAdapts.end(); nas_it++) {
		localLocList.push_back(ipv4::CLocatorValue((*nas_it)->getProperty<CStringValue>(INetAdapt::p_addr)->value()));

	}

	list<CSimpleRoutingNetlet_Header_RIX::ForwardingInfo>::iterator it;
	for (it = rix.nodes.begin(); it != rix.nodes.end(); it++) {

		bool isLocalLoc = false;
		list<ipv4::CLocatorValue>::const_iterator llit;
		for (llit = localLocList.begin(); llit != localLocList.end(); llit++) {
			if (*llit == it->loc) {
				isLocalLoc = true;
				break;
			}
		}

		if (!isLocalLoc) {
			SimpleFib::iterator fibit = fib.find(it->loc);
			if (fibit == fib.end()) {
				// new entry
				fib[it->loc] = SimpleFib_Entry(it->loc, nextHopLoc, na, it->hops + 1);
				DBG_INFO(FMT("Added %1% to FIB") % it->loc.toStr());

			} else {
				// we already have the entry, so update it if necessary
				if (it->hops + 1 < fibit->second.hops) {
					fibit->second.hops = it->hops + 1;
					fibit->second.netAdapt = na;
					fibit->second.nextHopLoc = nextHopLoc;
					fibit->second.timeStamp = nena->getSysTime();
				}
			}
		}

	}

	multiplexer->dbgPrintFib();
}

/**
 * @brief	Returns the Netlet's meta data
 */
INetletMetaData* CSimpleRoutingNetlet::getMetaData() const
{
	return (INetletMetaData*) metaData;
}

/**
 * @brief	Send out RIX messages
 */
void CSimpleRoutingNetlet::handleTimeout()
{
	// get a list with all net adapts we could use
	list<INetAdapt*>& nas = nena->getNetAdaptBroker()->getNetAdapts(getMetaData()->getArchName());

//	DBG_DEBUG(FMT("CSimpleRoutingNetlet: found %1% NAs") % nas.size());

	// send an RIX message over all those netAdapts
	list<INetAdapt *>::const_iterator nas_it;
	for (nas_it = nas.begin(); nas_it != nas.end(); nas_it++)
	{
		ipv4::CLocatorValue myloc((*nas_it)->getProperty<CStringValue>(INetAdapt::p_addr)->value());
		assert(myloc.isValid());

		CSimpleRoutingNetlet_Header_RIX rix;
		rix.nodes.push_back(CSimpleRoutingNetlet_Header_RIX::ForwardingInfo(myloc, 0)); // we are always reachable

		SimpleFib& fib = multiplexer->getFib();
		SimpleFib::const_iterator fib_it;
		for (fib_it = fib.begin(); fib_it != fib.end(); fib_it++) {
//			if (fib_it->second.netAdapt != *nas_it)
			rix.nodes.push_back(CSimpleRoutingNetlet_Header_RIX::ForwardingInfo(fib_it->first, fib_it->second.hops));

		}

		shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(this, next, IMessage::t_outgoing));
		pkt->setProperty(IMessage::p_srcId, new CStringValue(nena->getNodeName()));
		pkt->setProperty(IMessage::p_srcLoc, new ipv4::CLocatorValue(myloc)); // not necessary, but we already have it
		pkt->setProperty(IMessage::p_netletId, new CStringValue(getMetaData()->getId()));
		pkt->setProperty(IMessage::p_netAdapt, new CPointerValue<INetAdapt>(*nas_it));
		pkt->push_header(rix);

//		DBG_DEBUG(FMT("%1%: Sending RIX over %2%") % getId() % (*nas_it)->getId());
		sendMessage(pkt);
	}

	// next RIX cycle in 2 seconds // TODO: add parameter/option for this?
	scheduler->setTimer(new CSimpleRoutingNetlet_Timeout(2.0 + 2 * nena->getSys()->random(), this));
}

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleRoutingNetletMetaData::CSimpleRoutingNetletMetaData(const std::string& archName) :
		archName(archName)
{
	netletId = archName;
	netletId.replace(0, netletId.find(':'), "netlet");
	netletId += "/SimpleRoutingNetlet";

	MultiplexerFactories::iterator it = multiplexerFactories.find(getArchName());
	if (it == multiplexerFactories.end())
		DBG_ERROR(FMT("%1% cannot be instantiated: %2% not found") % getId() % getArchName());
	else
		netletFactories[getArchName()][getId()] = (INetletMetaData*) this;
}

/**
 * Destructor
 */
CSimpleRoutingNetletMetaData::~CSimpleRoutingNetletMetaData()
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
std::string CSimpleRoutingNetletMetaData::getArchName() const
{
	return archName;
}

/**
 * Return Netlet name
 */
const std::string& CSimpleRoutingNetletMetaData::getId() const
{
	return netletId;
}

/**
 * @brief	Returns true if the Netlet does not offer any transport service
 * 			for applications, false otherwise.
 */
bool CSimpleRoutingNetletMetaData::isControlNetlet() const
{
	return true;
}

/**
 * Create an instance of the Netlet
 */
INetlet* CSimpleRoutingNetletMetaData::createNetlet(CNena *nodeA, IMessageScheduler *sched)
{
	return new CSimpleRoutingNetlet(this, nodeA, sched);
}

/* ========================================================================= */

/**
 * @brief	For demo-purposes and tests: instantiate multiple architectures
 */
class CSimpleRoutingNetletMultiplier
{
public:
	list<CSimpleRoutingNetletMetaData*> factories;

	CSimpleRoutingNetletMultiplier()
	{
		list<string>::const_iterator it;
		for (it = simpleArchitectures.begin(); it != simpleArchitectures.end(); it++)
			factories.push_back(new CSimpleRoutingNetletMetaData(*it));
	}

	virtual ~CSimpleRoutingNetletMultiplier()
	{
		list<CSimpleRoutingNetletMetaData*>::iterator it;
		for (it = factories.begin(); it != factories.end(); it++) {
			delete *it;
		}
		factories.clear();
	}
};

/**
 * Initializer for shared library. Registers factories.
 */
static CSimpleRoutingNetletMultiplier simpleRoutingNetletMultiplier;

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
