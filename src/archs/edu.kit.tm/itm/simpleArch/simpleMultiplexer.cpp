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

#include "simpleMultiplexer.h"

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

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

/* ========================================================================= */

/**
 * @brief	Minimum header containing the source and destination IDs
 *
 * Header format is [A][B...][C][D...][EEEE][FFFF][GG][HHHH][II]
 * A is the string length of destNodeName (one byte)
 * B is the string of destNodeName (variable length)
 * C is the string length of srcNodeName (one byte)
 * D is the string of srcNodeName (variable length)
 * E is the hash of the Netlet identifier (four bytes)
 * F is the destination IPv4 address (four bytes)
 * G is the destination IP port (two bytes)
 * H is the source IPv4 address (four bytes)
 * I is the source IP port (two bytes)
 */
class SimpleMultiplexer_Header : public IHeader
{
public:
	std::string destNodeName;
	std::string srcNodeName;
	nena::hash_t netletHash;
	nena::hash_t serviceHash;
	nena::hash_t destFlowHash;
	nena::hash_t srcFlowHash;
	ipv4::CLocatorValue destIpv4Addr;
	ipv4::CLocatorValue srcIpv4Addr;
	bool autoForward;

	SimpleMultiplexer_Header() :
		netletHash(0), serviceHash(0),
		destFlowHash(0), srcFlowHash(0),
		destIpv4Addr(0), srcIpv4Addr(0),
		autoForward(true)
	{};

	SimpleMultiplexer_Header(const SimpleMultiplexer_Header & rhs) :
			IHeader()
	{
		destNodeName = rhs.destNodeName;
		srcNodeName = rhs.srcNodeName;
		netletHash = rhs.netletHash;
		serviceHash = rhs.serviceHash;
		destFlowHash = rhs.destFlowHash;
		srcFlowHash = rhs.srcFlowHash;
		destIpv4Addr = rhs.destIpv4Addr;
		srcIpv4Addr = rhs.srcIpv4Addr;
		autoForward = rhs.autoForward;
	};

	inline size_t calcSize() const
	{
		return destNodeName.size() +
			srcNodeName.size() +
			sizeof(unsigned char)*2 +
			sizeof(uint32_t)*2 +
			sizeof(uint32_t)*2 +
			sizeof(uint32_t)*2 +
			sizeof(uint16_t)*2 +
			sizeof(unsigned char);
	}

	/**
	 * @brief 	Serialize all relevant data into a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		assert(destNodeName.size() <= 255);
		assert(srcNodeName.size() <= 255);

		boost::shared_ptr<CMessageBuffer> mbuf(new CMessageBuffer(calcSize()));

		mbuf->push_uchar((unsigned char) destNodeName.size());
		mbuf->push_string(destNodeName);

		mbuf->push_uchar((unsigned char) srcNodeName.size());
		mbuf->push_string(srcNodeName);

		mbuf->push_ulong((uint32_t) netletHash);
		mbuf->push_ulong((uint32_t) serviceHash);
		mbuf->push_ulong((uint32_t) destFlowHash);
		mbuf->push_ulong((uint32_t) srcFlowHash);

		mbuf->push_ulong(destIpv4Addr.getAddr());
		mbuf->push_ushort(destIpv4Addr.getPort());
		mbuf->push_ulong(srcIpv4Addr.getAddr());
		mbuf->push_ushort(srcIpv4Addr.getPort());

		mbuf->push_uchar((unsigned char) autoForward);

		return mbuf;
	}

	/**
	 * @brief 	De-serialize all relevant data from a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> mbuf)
	{
		unsigned char l;

		l = mbuf->pop_uchar();
		destNodeName = mbuf->pop_string(l);

		l = mbuf->pop_uchar();
		srcNodeName = mbuf->pop_string(l);

		netletHash = (nena::hash_t) mbuf->pop_ulong();
		serviceHash = (nena::hash_t) mbuf->pop_ulong();
		destFlowHash = (nena::hash_t) mbuf->pop_ulong();
		srcFlowHash = (nena::hash_t) mbuf->pop_ulong();

		destIpv4Addr.setAddr(mbuf->pop_ulong());
		destIpv4Addr.setPort(mbuf->pop_ushort());
		srcIpv4Addr.setAddr(mbuf->pop_ulong());
		srcIpv4Addr.setPort(mbuf->pop_ushort());

		autoForward = mbuf->pop_uchar();
	}
};

/* ========================================================================= */

/**
 * @brief	Constructor
 */
CSimpleNameAddrMapper::CSimpleNameAddrMapper(const std::string& archName, CNena* nodeA, IMessageScheduler *sched) :
	INameAddrMapper(nodeA, sched), archName(archName)
{
	string id = archName;
	id = id.replace(0, id.find(':'), "addrMapper");
	setId(id);

	registerEvent(EVENT_SIMPLEARCH_RESOLVELOCATORREQ);
	registerEvent(EVENT_SIMPLEARCH_RESOLVELOCATORRESP);
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleNameAddrMapper::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> ev = msg->cast<IEvent>();
	if (ev->getId() == EVENT_SIMPLEARCH_RESOLVELOCATORRESP) {
		shared_ptr<Event_ResolveLocatorResponse> resp = ev->cast<Event_ResolveLocatorResponse>();

		if (resp->uid.size() == 16 && resp->name.size() > 0 && resp->locators.list().size() > 0 &&
				resp->locators.list().front()->isValid()) {
//			DBG_DEBUG(FMT("Received %1%, updating id/locator mapping for %2% (%3% -> %4%)...")
//				% ev->getId() % resp->name % resp->uid.toStr() % resp->locator.toStr());

			list<shared_ptr<ipv4::CLocatorValue> >::const_iterator rllit;
			for (rllit = resp->locators.list().begin(); rllit != resp->locators.list().end(); rllit++) {
				bool found = false;
				list<shared_ptr<ipv4::CLocatorValue> >::const_iterator llit;
				for (llit = resolveMap[resp->name].list().begin(); llit != resolveMap[resp->name].list().end(); llit++) {
					if (*llit == *rllit) {
						//DBG_DEBUG(FMT("id/locator mapping for %1% (%2% -> %3%) already known")
						//	% resp->name % resp->uid.toStr() % rllit->toStr());
						found = true;
						break;

					}

				}

				if (!found) {
					resolveMap[resp->name].list().push_back(*rllit);
					//DBG_DEBUG(FMT("adding new id/locator mapping for %1% (%2% -> %3%)")
					//	% resp->name % resp->uid.toStr() % rllit->toStr());

				}

			}

		} else {
			throw EUnhandledMessage((FMT("Invalid event message content (event %1%, sender %2%, uid-size %3%, name %4%, loclistsize %5%, 1stloc %6%)")
				% ev->getId() % ev->getFrom()->getClassName()
				% resp->uid.size() % resp->name % resp->locators.list().size()
				% resp->locators.list().front()->toStr()).str());

		}

	} else {
		DBG_DEBUG(FMT("Received unknown event message %1% from %2%")
			% ev->getId() % ev->getFrom()->getClassName());
		throw EUnhandledMessage("Unhandled event message");

	}
}

/**
 * @brief	Destructor
 */
CSimpleNameAddrMapper::~CSimpleNameAddrMapper()
{
}

/**
 * @brief	Load name/addr mapping table from file. The file format is
 * 			<name>\t<ip:port> whereas <:port> is optional.
 */
void CSimpleNameAddrMapper::loadResolverConfig()
{
	unsigned int count = 0;

	vector<vector<string> > resolverList;

	if (nodeArch->getConfig()->hasParameter(getId(), "resolverList", XMLFile::STRING, XMLFile::TABLE))
		nodeArch->getConfig()->getParameterTable(getId(), "resolverList", resolverList);

	for (size_t i=0; i < resolverList.size (); i++)
	{
		string name = resolverList[i][0];
		string address = resolverList[i][1];
		string port = resolverList[i][2];
		string combinedAddr = address + string(":") + port;

		shared_ptr<ipv4::CLocatorValue> lv(new ipv4::CLocatorValue(combinedAddr));
		if (!lv->isValid())
		{
			DBG_ERROR(FMT("%1%: format error (in IP address): %2%") % getId() % combinedAddr);
		}
		else
		{
			if (name.empty())
			{
				DBG_ERROR(FMT("%1%: format error (in node name) (empty)") % getId());
			}
			else
			{
				list<shared_ptr<ipv4::CLocatorValue> >::iterator it = resolveMap[name].list().begin ();
				
				for (; it != resolveMap[name].list().end() && **it != *lv; it++);
				
				if (it == resolveMap[name].list().end())
				{
					resolveMap[name].list().push_back(lv);
					DBG_INFO(FMT("%1%: new resolver Entry: %2% - %3%:%4%.") % getId() % name % address % port );
					count++;
				}
			}
		}
	}

	DBG_DEBUG(FMT("%1%: Added %2% resolver entries.") % getId() % count);

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
void CSimpleNameAddrMapper::getPotentialNetlets(IAppConnector* appc, std::list<INetletMetaData*>& netletList) const
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

/**
 * @brief	Resolve a name into an address
 */
ipv4::CLocatorValue CSimpleNameAddrMapper::resolve(const std::string & name)
{
	notifyListeners(Event_ResolveLocatorRequest(name, resolveId(name)));
//	DBG_DEBUG(FMT("Resolving %1% (%2%)...") % name % md5.hexdigest());

	map<string, ipv4::CLocatorList>::iterator it;
	it = resolveMap.find(name);
	if (it != resolveMap.end()) {
//		DBG_DEBUG(FMT("CSimpleNameAddrMapper::resolve(): Found %1%!") % name);
		return *it->second.list().front();

	} else {
		DBG_WARNING(FMT("%1%::resolve(): Cannot find %2%!") % getId() % name);
		return ipv4::CLocatorValue(); // return an invalid value

	}
}

/**
 * @brief	Resolve an address into a name (reverse look-up).
 */
std::string CSimpleNameAddrMapper::resolve(ipv4::CLocatorValue loc)
{
	map<string, ipv4::CLocatorList>::iterator it;
	for(it = resolveMap.begin(); it != resolveMap.end(); it++) {
		list<shared_ptr<ipv4::CLocatorValue> >::const_iterator lit;
		for (lit = it->second.list().begin(); lit != it->second.list().end(); lit++) {
			if (*(*lit) == loc)
				return it->first;
		}
	}

	return std::string();
}

set<string> CSimpleNameAddrMapper::getURLs ()
{
	set<string> ret;
	map<string, ipv4::CLocatorList>::iterator it;
	for(it = resolveMap.begin(); it != resolveMap.end(); it++)
		ret.insert (it->first);

	return ret;
}

void CSimpleNameAddrMapper::addBroadcast (ipv4::CLocatorValue loc)
{
	string name = "node://broadcast";
	list<shared_ptr<ipv4::CLocatorValue> >::iterator it = resolveMap[name].list().begin ();
				
				for (; it != resolveMap[name].list().end() && **it != loc; it++);
				
				if (it == resolveMap[name].list().end())
	{
		shared_ptr<ipv4::CLocatorValue> lv(new ipv4::CLocatorValue(loc));
		resolveMap[name].list().push_back(lv);
	}
}

void CSimpleNameAddrMapper::addLocator (string name, ipv4::CLocatorValue loc)
{
	if (!resolveMap[name].list().empty())
		resolveMap[name].list().clear();

	shared_ptr<ipv4::CLocatorValue> lv(new ipv4::CLocatorValue(loc));
	resolveMap[name].list().push_back(lv);
}

/**
 * @brief	Resolve a name into a list of locators.
 */
ipv4::CLocatorList CSimpleNameAddrMapper::resolveAll(const std::string& name)
{
	notifyListeners(Event_ResolveLocatorRequest(name, resolveId(name)));
//	DBG_DEBUG(FMT("Resolving %1% (%2%)...") % name % md5.hexdigest());

	map<std::string, ipv4::CLocatorList>::const_iterator it;
	it = resolveMap.find(name);
	if (it != resolveMap.end()) {
//		DBG_DEBUG(FMT("CSimpleNameAddrMapper::resolve(): Found %1%!") % name);
		return it->second;

	} else {
		DBG_ERROR(FMT("%1%: Cannot find %2%!") % getId() % name);
		return ipv4::CLocatorList(); // return an empty list

	}
}

/**
 * @brief	Maps name to identifier (currently 128bit MD5)
 */
shared_buffer_t CSimpleNameAddrMapper::resolveId(const std::string& name)
{
	MD5 md5(name);

	return shared_buffer_t((const char*) md5.digestBuf(), 16);
}

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleMultiplexer::CSimpleMultiplexer(IMultiplexerMetaData *metaData, CNena *nodeA, IMessageScheduler *sched)
	:INetletMultiplexer(metaData, nodeA, sched)
{
	className += "::CSimpleMultiplexer";

	string id = metaData->getArchName();
	id = id.replace(0, id.find(':'), "multiplexer");
	setId(id);

	nameAddrMapper = new CSimpleNameAddrMapper(metaData->getArchName(), nodeArch, sched);
	nameAddrMapper->loadResolverConfig();

	netletSelector = static_cast<CNetletSelector*>(nodeA->lookupInternalService("internalservice://nena/netletSelector"));
	assert(netletSelector != NULL);

	netletSelector->registerNameAddrMapper(nameAddrMapper);

	registerEvent(EVENT_SIMPLEARCH_UNHANDLEDPACKET);

	reactiveRouting = false;

	// transitional
	localAddr = nameAddrMapper->resolve(nodeArch->getNodeName());
}

/**
 * Constructor
 */
CSimpleMultiplexer::~CSimpleMultiplexer()
{
	// TODO: unregister name/addr mapper?
	delete nameAddrMapper;
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleMultiplexer::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<Management::Event_UpdatedConfig> ev = msg->cast<Management::Event_UpdatedConfig>();
	if (ev.get() == NULL)
		throw EUnhandledMessage();
	else
		nameAddrMapper->loadResolverConfig();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CSimpleMultiplexer::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CSimpleMultiplexer::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	shared_ptr<CMessageBuffer> mbuf = msg->cast<CMessageBuffer>();
	assert(mbuf != NULL);

	shared_ptr<CMorphableValue> mv;
	ipv4::CLocatorValue loc;
	SimpleMultiplexer_Header hdr;
	bool isBroadcast = false;

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

	// source ID
	try {
		hdr.srcNodeName = msg->getProperty<CStringValue>(IMessage::p_srcId)->value();
	} catch (exception& e) {
		// here, we should have an ID
		DBG_ERROR("CSimpleMultiplexer: Cannot send outgoing packet without a source ID");
		throw EUnhandledMessage("Cannot send outgoing packet without a source ID");
	}

	// destination ID
	try {
		hdr.destNodeName = msg->getProperty<CStringValue>(IMessage::p_destId)->value();
	} catch (exception& e) {
		// ignore; this means that the packet will be put on the default
		// interface without a specific destination address
		isBroadcast = true;
	}

	if (!hdr.destNodeName.empty() and hdr.destNodeName.compare(0, 7, "node://") != 0) {
		string requestUri = hdr.destNodeName;
		size_t n = requestUri.find(':');
		if (n == string::npos || requestUri.compare(n, 3, "://") != 0) {
			DBG_ERROR(FMT("%1%: malformed URI: %2%") % getId() % requestUri);
			throw EUnhandledMessage("malformed URI");
		}

		n = requestUri.find('/', n+3);
		if (n == string::npos) {
			DBG_ERROR(FMT("%1%: no service ID part in URI: %2%") % getId() % requestUri);
			throw EUnhandledMessage("no service ID part in URI");
		}

		n = requestUri.find('/', n+1);
		if (n != string::npos) {
			// stripping everything beyond service ID part
			requestUri.erase(n);
		}

		hdr.serviceHash = nena::hash_string(requestUri);

		n = hdr.destNodeName.find(':');
		if (n != string::npos) {
			hdr.destNodeName.replace(0, n, "node");
		}
		n = hdr.destNodeName.find('/', 8);
		if (n != string::npos) {
			hdr.destNodeName.erase(n, hdr.destNodeName.size()-n);
		}
		n = hdr.destNodeName.find('.');
		if (n != string::npos) {
			hdr.destNodeName.replace(n, 1, "/");
		}

//		DBG_DEBUG(FMT("%1%: extracted service %2%, destNodeName %3%") % getId() % requestUri % hdr.destNodeName);
	}

	if (hdr.destNodeName == "node://broadcast") {
		hdr.destNodeName = "";
		isBroadcast = true;
	}

	// netlet hash
	try {
		hdr.netletHash = nena::hash_string(msg->getProperty<CStringValue>(IMessage::p_netletId)->value());
	} catch (exception& e) {
		// here, we should have an ID
		DBG_ERROR("CSimpleMultiplexer: Cannot send outgoing packet without a Netlet ID");
		throw EUnhandledMessage("Cannot send outgoing packet without a Netlet ID");
	}

//	DBG_DEBUG(FMT("CSimpleMultiplexer: Outgoing message from Netlet %1% (%2$08x)") %
//			msg->getProperty<CStringValue>(IMessage::p_netletId)->value() %
//			hdr.netletHash);

	// src flow hash
	if (msg->getFlowState()) {
		hdr.destFlowHash = msg->getFlowState()->getRemoteFlowId(); // may be 0
		hdr.srcFlowHash = msg->getFlowState()->getFlowId(); // maybe create a real hash some day...
	}

	// destination locator
	ipv4::CLocatorList destLocList;
	if (!isBroadcast) {
		if (msg->hasProperty(IMessage::p_destLoc, mv)) {
			loc = *mv->cast<ipv4::CLocatorValue>();
			if (!loc.isValid()) {
				// try to resolve dest locator
				destLocList = nameAddrMapper->resolveAll(hdr.destNodeName);

			} else {
				shared_ptr<ipv4::CLocatorValue> locv(new ipv4::CLocatorValue(loc));
				destLocList.list().push_back(locv);

			}

		} else {
			// try to resolve dest locator
			destLocList = nameAddrMapper->resolveAll(hdr.destNodeName);
		}

		if (destLocList.list().empty())
			throw EUnhandledMessage("Cannot send outgoing packet without a destination ipv4::CLocatorValue");

	} // else broadcast

	if (msg->hasProperty(IMessage::p_autoForward, mv))
		hdr.autoForward = mv->cast<CBoolValue>()->value();

//	if (hdr.srcFlowHash != 0 or hdr.destFlowHash != 0) {
//		DBG_INFO(FMT("%1%: send flow [%2%(%3%), %4%(%5%)]") % getId() %
//				hdr.srcNodeName % hdr.srcFlowHash %
//				hdr.destNodeName % hdr.destFlowHash);
//	}

	mbuf->setFrom(this);

	if (!destLocList.list().empty()) {
		// determine outgoing interface from FIB
		INetAdapt* netAdapt = NULL;

		// check whether a netadapt is forced via flow state
		if (msg->getFlowState()) {
			try {
				netAdapt = msg->getFlowState()->getProperty<CPointerValue<INetAdapt> >(IMessage::p_netAdapt)->value();
			} catch (...) {
				// nothing
			}
		}

		// check whether a netadapt is forced via message
		if (netAdapt == NULL) {
			try {
				netAdapt = msg->getProperty<CPointerValue<INetAdapt> >(IMessage::p_netAdapt)->value();
			} catch (...) {
				// nothing
			}
		}

		// iterate through locator list
		bool sent = false;
		list<shared_ptr<ipv4::CLocatorValue> >::iterator dit;
		for (dit = destLocList.list().begin(); dit != destLocList.list().end(); dit++) {
			SimpleFib::iterator it;
			it = fib.find(*(*dit));
			if (it != fib.end()) {
				if (netAdapt == it->second.netAdapt || netAdapt == NULL) {
					// forced netAdapt or first entry
					hdr.srcIpv4Addr = ipv4::CLocatorValue(it->second.netAdapt->getProperty<CStringValue>(INetAdapt::p_addr)->value());
					hdr.destIpv4Addr = it->first;

					if (it->second.nextHopLoc.isValid())
						msg->setProperty(IMessage::p_nextHopLoc, new ipv4::CLocatorValue(it->second.nextHopLoc));

					mbuf->push_header(hdr);
					mbuf->setTo(it->second.netAdapt);
					sendMessage(mbuf);
					sent = true;

					// Update the timestamp for this FIB entry
					updateTimeStamp(it->second.destLoc);
					break;

				}

			}

		}

		if (!sent) {
			string m;
			if (destLocList.list().empty()) {
				m = str(FMT("%1%: No locator found for %2%") % getId() % hdr.destNodeName);
			} else {
				m = str(FMT("%1%: No FIB entry found for any locator of %2%") % getId() % hdr.destNodeName);
			}
			DBG_INFO(m);

			if (!reactiveRouting) {
				throw EUnhandledMessage(m);

			} else {
				// send the message to the AODV protocol
				// TODO: generalize this (works only fpr AODV at the moment)
				// TODO: most probably, we add some queuing to the multiplexer itself
				INetlet* aodvNetlet = nodeArch->getInstanceOf("netlet://es.robotiker/simpleArch/SimpleAodvNetlet");
				if (aodvNetlet == NULL) {
					m = str(FMT("%1%: Reactive routing enabled, but AODV Netlet not found!") % getId());
					throw EUnhandledMessage(m);

				} else {
					msg->setType(IMessage::t_incoming);
					msg->setFrom(this);
					msg->setTo((IMessageProcessor*) aodvNetlet);

//					DBG_DEBUG(FMT("CSimpleMultiplexer: mbuf %1%/%2% (from %3%)")
//								% (int) mbuf->getBuffer().length() % mbuf->size()
//								% mbuf->getFrom()->getClassName());

					sendMessage(msg);

				}

			}

		}

	} else {
		// broadcast address
		hdr.destIpv4Addr = ipv4::CLocatorValue();
		list<INetAdapt*> netAdapts = nodeArch->getNetAdaptBroker()->getNetAdapts(getMetaData()->getArchName());

		// check whether a netadapt is forced via message
		INetAdapt* netAdapt = NULL;
		if (netAdapt == NULL) {
			try {
				netAdapt = msg->getProperty<CPointerValue<INetAdapt> >(IMessage::p_netAdapt)->value();
			} catch (...) {
				// nothing
			}
		}

		if (netAdapts.size() > 0) {
			list<INetAdapt*>::iterator it;
			for (it = netAdapts.begin(); it != netAdapts.end(); it++) {
				if (*it == netAdapt || netAdapt == NULL) {
					shared_ptr<CMessageBuffer> mbuf2 = mbuf->clone();
					hdr.srcIpv4Addr = ipv4::CLocatorValue((*it)->getProperty<CStringValue>(INetAdapt::p_addr)->value());
					shared_ptr<ipv4::CLocatorList> destLocs = (*it)->getProperty<ipv4::CLocatorList>(INetAdapt::p_broadcast);
					mbuf2->setProperty(IMessage::p_multiDestLoc, destLocs);
					mbuf2->push_header(hdr);
					mbuf2->setTo(*it);
					sendMessage(mbuf2);

				}

			}

		} else {
			string m("no network adaptor found");
			throw EUnhandledMessage(m);

		}

	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CSimpleMultiplexer::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	shared_ptr<CMessageBuffer> mbuf = msg->cast<CMessageBuffer>();
	if (mbuf == NULL) throw EUnhandledMessage("Incoming message not of type CMessageBuffer.");
	
	SimpleMultiplexer_Header hdr;
	mbuf->peek_header(hdr);

	mbuf->setProperty(IMessage::p_destId, new CStringValue(hdr.destNodeName));
	mbuf->setProperty(IMessage::p_srcId, new CStringValue(hdr.srcNodeName));

//	if (hdr.srcFlowHash != 0 or hdr.destFlowHash != 0) {
//		DBG_INFO(FMT("%1%: recv flow [%2%(%3%), %4%(%5%)]") % getId() %
//					hdr.srcNodeName % hdr.srcFlowHash %
//					hdr.destNodeName % hdr.destFlowHash);
//	}

//	DBG_INFO(FMT("Received packet from %1% (dest %2%)") % hdr.srcNodeName % hdr.destNodeName);

	if (hdr.autoForward && hdr.destNodeName != "" && hdr.destNodeName != nodeArch->getNodeName()) {
		// relaying
		map<ipv4::CLocatorValue, SimpleFib_Entry>::iterator it;
		it = fib.find(hdr.destIpv4Addr);
		if (it != fib.end()) {
//			DBG_DEBUG(FMT("%1% CSimpleMultiplexer: Relaying message (src %2%, dest %3%, nextHop %4%)...") %
//				nodeArch->getNodeName() %
//				hdr->srcIpv4Addr.toStr() %
//				hdr->destIpv4Addr.toStr() %
//				it->second.nextHopLoc.toStr());

			mbuf->flushVisitedProcessors(); // reset loop detection

			mbuf->setProperty(IMessage::p_destLoc, new ipv4::CLocatorValue(hdr.destIpv4Addr));
			mbuf->setProperty(IMessage::p_srcLoc, new ipv4::CLocatorValue(hdr.srcIpv4Addr));

			if (it->second.nextHopLoc.isValid())
				mbuf->setProperty(IMessage::p_nextHopLoc, new ipv4::CLocatorValue(it->second.nextHopLoc));

			mbuf->setType(IMessage::t_outgoing);
			mbuf->setFrom(this);
			mbuf->setTo(it->second.netAdapt);
			sendMessage(mbuf);

			// Update the timestamp for this FIB entry
			updateTimeStamp(hdr.destIpv4Addr);

		} else {
			DBG_ERROR(FMT("%1% CSimpleMultiplexer: Could not relay message (src %2% [%4%], dest %3% [%5%]), discarding: no entry in FIB") %
				nodeArch->getNodeName() %
				hdr.srcNodeName %
				hdr.destNodeName %
				hdr.srcIpv4Addr.addrToStr() %
				hdr.destIpv4Addr.addrToStr());

		}

	} else {
		// local delivery
		mbuf->remove_front(hdr.calcSize());

		// determine Netlet name
		map<nena::hash_t, INetlet*>::iterator it = hashedNetlets.find(hdr.netletHash);
		if (it == hashedNetlets.end()) {

			string m = str(FMT("%1% %2%: Received message (src %3%, dest %4%) with unknown Netlet hash (%5$08x)") %
					nodeArch->getNodeName() % getId() %
					hdr.srcNodeName %
					hdr.destNodeName %
					hdr.netletHash);
			DBG_DEBUG(m);

//			DBG_DEBUG(FMT("Message: %1%") % mbuf->to_str());

			for (it = hashedNetlets.begin(); it != hashedNetlets.end(); it++) {
				DBG_DEBUG(FMT("  Known Netlet: %1% (%2$08x)") % it->second->getMetaData()->getId() % it->first);
			}

			notifyListeners(Event_UnhandledPacket(msg));
			throw EUnhandledMessage(m);
		}

//		DBG_INFO(FMT("Received packet for %1%") % it->second->getMetaData()->getId());

		if (hdr.destFlowHash != 0) {
			// look up flow ID
			shared_ptr<CFlowState> flowState = nodeArch->getFlowState(hdr.destFlowHash);
			if (flowState != NULL) {
				mbuf->setFlowState(flowState);
				flowState->setRemoteFlowId(hdr.srcFlowHash);

			} else {
				string m = (FMT("%1%: invalid local flow ID %2%") % getId() % hdr.destFlowHash).str();
				DBG_DEBUG(m);
				throw EUnhandledMessage(m);

			}

		}

		if (hdr.serviceHash != 0) {
			std::string serviceId;
			list<std::string> services;
			netletSelector->getRegisteredAppServices(services);
			list<std::string>::const_iterator it;
			for (it = services.begin(); it != services.end(); it++)
			{
				if (nena::hash_string(*it) == hdr.serviceHash) {
					serviceId = *it;
					mbuf->setProperty(IMessage::p_serviceId, new CStringValue(serviceId));
					break;
				}
			}

			if (serviceId.empty())
				throw EUnhandledMessage((FMT("%1%: Service hash could not be resolved") % getId()).str());

			if (mbuf->getFlowState() == NULL) {
				CFlowState::FlowId flowId = netletSelector->lookupAppService(serviceId);
				if (flowId == 0)
					throw EUnhandledMessage("Could not find a local flow ID");
				shared_ptr<CFlowState> fs = netletSelector->lookupFlowId(flowId)->getFlowState();

				// check for a child flow state established for the remote flow state
				shared_ptr<CFlowState> childfs = fs->getChildFlowState(hdr.srcNodeName, hdr.srcFlowHash);
				if (childfs != NULL) {
					mbuf->setFlowState(childfs);
//					DBG_DEBUG(FMT("%1%: found child flow [%2%(%3%), %4%(%5%)]") % getId() %
//							childfs->getRemoteId() % childfs->getRemoteFlowId() %
//							nodeArch->getNodeName() % childfs->getFlowId());

				} else {
					childfs = nodeArch->createFlowState(NULL);
					childfs->setLocalId(nodeArch->getNodeName());
					childfs->setRemoteId(hdr.srcNodeName);
					childfs->setServiceId(serviceId);
					childfs->setRemoteFlowId(hdr.srcFlowHash);
					fs->addChildFlowState(childfs);

					mbuf->setFlowState(childfs);
					DBG_DEBUG(FMT("%1%: new child flow for [%2%(%3%)]") % getId() % hdr.srcNodeName % hdr.srcFlowHash);

				}

			}

		}

		shared_ptr<CFlowState> fs = mbuf->getFlowState();
		if (fs) { // NULL for control netlets w/o flow states
			fs->incInFloatingPackets();

//			if (fs->getInFloatingPackets() >= fs->getInMaxFloatingPackets()) {
//				DBG_WARNING(FMT("%1%: exceeding maximum allowed number of incoming floating packets (%2% / %3%)") %
//						getId() % fs->getInFloatingPackets() % fs->getInMaxFloatingPackets());
//
//			}

		}

		mbuf->setProperty(IMessage::p_destLoc, new ipv4::CLocatorValue(hdr.destIpv4Addr));
		mbuf->setProperty(IMessage::p_srcLoc, new ipv4::CLocatorValue(hdr.srcIpv4Addr));
		mbuf->setProperty(IMessage::p_netletId, new CStringValue(it->second->getMetaData()->getId()));
		mbuf->setFrom(this);
		mbuf->setTo((IMessageProcessor*) it->second);
		sendMessage(mbuf);

		// Update the timestamp for this FIB entry
		updateTimeStamp(hdr.destIpv4Addr);

	}
}

/**
 * @brief	Called if new Netlets of this architecture were added to the system.
 */
void CSimpleMultiplexer::refreshNetlets()
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

/**
 * @brief	Print the FIB for debugging purposes
 */
void CSimpleMultiplexer::dbgPrintFib()
{
	DBG_INFO(FMT("=== FIB for %1% %2%") % nodeArch->getNodeName() % getMetaData()->getArchName());
	map<ipv4::CLocatorValue, SimpleFib_Entry>::iterator it;
	for (it = fib.begin(); it != fib.end(); it++) {
		assert(it->second.netAdapt != NULL);

		shared_ptr<CMorphableValue> mv;
		string netAdaptName;
		if (it->second.netAdapt->hasProperty(INetAdapt::p_name, mv))
			netAdaptName = mv->cast<CStringValue>()->value();

		DBG_INFO(FMT("  %1% via %2% (nextHop %3%, hops %4%, timeStamp %5%, valid %6%)") %
			it->second.destLoc.toStr() %
			netAdaptName %
			it->second.nextHopLoc.toStr() %
			it->second.hops %
			it->second.timeStamp %
			it->first.isValid());
	}
}

SimpleFib & CSimpleMultiplexer::getFib()
{
	return fib;
}

/**
 * @brief	Adds a FIB entry.
 */
void CSimpleMultiplexer::addFibEntry(ipv4::CLocatorValue dest, ipv4::CLocatorValue next, int hops, INetAdapt* na)
{
	assert(dest != localAddr);

	map<ipv4::CLocatorValue, SimpleFib_Entry>::const_iterator it;
	it = fib.find(dest);

	if(it == fib.end())
		fib[dest] = SimpleFib_Entry(dest, next, na, hops, nodeArch->getSysTime());
}

/**
 * @brief	Adds a FIB entry.
 */
void CSimpleMultiplexer::addFibEntry(const SimpleFib_Entry& entry)
{
	assert(entry.destLoc != localAddr);
	assert(entry.destLoc.isValid());

	fib[entry.destLoc] = entry;
}

/**
 * @brief	Delete a FIB entry.
 */
void CSimpleMultiplexer::delFibEntry(ipv4::CLocatorValue dest)
{
	map<ipv4::CLocatorValue, SimpleFib_Entry>::const_iterator it;
	it = fib.find(dest);

	if(it != fib.end())
		fib.erase(dest);
}

/**
 * @brief	Invalidate the FIB completely.
 */
void CSimpleMultiplexer::delFib()
{
	fib.clear();
}

/**
 * @brief	Update the timestamp for a FIB entry.
 */
void CSimpleMultiplexer::updateTimeStamp(ipv4::CLocatorValue dest)
{
	map<ipv4::CLocatorValue, SimpleFib_Entry>::const_iterator it;
	it = fib.find(dest);

	if(it != fib.end())
		fib[dest].timeStamp = nodeArch->getSysTime();
}

/**
 * @brief	Returns the timestamp of a FIB entry.
 */
double CSimpleMultiplexer::getTimeStamp(ipv4::CLocatorValue dest)
{
	map<ipv4::CLocatorValue, SimpleFib_Entry>::const_iterator it;
	it = fib.find(dest);

	if(it != fib.end())
		return fib[dest].timeStamp;
	else
		return 0;
}

/**
 * @brief	Enables or disables reactive routing support.
 */
void CSimpleMultiplexer::setReactiveRoutingEnabled(bool enabled)
{
	reactiveRouting = enabled;
}


CSimpleNameAddrMapper* CSimpleMultiplexer::getNameAddrMapper() const
{
	return nameAddrMapper;
}

string CSimpleMultiplexer::getStats ()
{
//	xml_document stats;
	string ret;
//	ostringstream oss;
//
//	/**
//	 * first fib
//	 */
//
//	stats.append_child ("stats").append_child ("component").append_attribute("name").set_value("fib");
//	xml_node fiblist = stats.child("stats").find_child_by_attribute("component", "name", "fib").append_child("fiblist");
//
//	/*
//	 * always add us
//	 */
//
//	xml_node li = fiblist.append_child("li");
//	li.append_child("destination").append_child(node_pcdata).set_value("localhost");
//
//	map<ipv4::CLocatorValue, SimpleFib_Entry>::iterator it;
//	for (it = fib.begin(); it != fib.end(); it++)
//	{
//		shared_ptr<CMorphableValue> mv;
//		string netAdaptName;
//		if (it->second.netAdapt->hasProperty(INetAdapt::p_name, mv))
//			netAdaptName = mv->cast<CStringValue>()->value();
//
//		/*DBG_INFO(FMT("  %1% via %2% (nextHop %3%, hops %4%, timeStamp %5%, valid %6%)") %
//			it->second.destLoc.toStr() %
//			netAdaptName %
//			it->second.nextHopLoc.toStr() %
//			it->second.hops %
//			it->second.timeStamp %
//			it->first.isValid());*/
//
//		xml_node li = fiblist.append_child("li");
//		li.append_child("destination").append_child(node_pcdata).set_value(it->second.destLoc.toStr().c_str());
//		li.append_child("adapter").append_child(node_pcdata).set_value(netAdaptName.c_str());
//	}
//
//	/**
//	 * then urls
//	 */
//
//	set<string> urls = nameAddrMapper->getURLs();
//
//	stats.child ("stats").append_child ("component").append_attribute("name").set_value("nameAddrMapper");
//	xml_node addrlist = stats.child("stats").find_child_by_attribute("component", "name", "nameAddrMapper").append_child("urllist");
//
//	set<string>::const_iterator cit;
//	for (cit = urls.begin(); cit != urls.end(); cit++)
//	{
//		list<shared_ptr<ipv4::CLocatorValue> > ips = nameAddrMapper->resolveAll(*cit).list();
//
//		for (list<shared_ptr<ipv4::CLocatorValue> >::const_iterator ip = ips.begin (); ip != ips.end(); ip++)
//		{
//			li = addrlist.append_child("li");
//			li.append_child("url").append_child(node_pcdata).set_value(cit->c_str());
//			li.append_child("addr").append_child(node_pcdata).set_value((*ip)->toStr().c_str());
//		}
//	}
//
//	stats.print(oss, "", format_raw);
//	ret = oss.str();
	return ret;
}

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleMultiplexerMetaData::CSimpleMultiplexerMetaData(const string& archName)
	: IMultiplexerMetaData(), archName(archName)
{
	MultiplexerFactories::iterator mit= multiplexerFactories.find(getArchName());

	if (mit != multiplexerFactories.end())
		DBG_ERROR(FMT("SimpleArch: FIXME: %1% already loaded! Skipping factory registration...") % getArchName());
	else
		multiplexerFactories[getArchName()] = (IMultiplexerMetaData*) this;
}

/**
 * Destructor
 */
CSimpleMultiplexerMetaData::~CSimpleMultiplexerMetaData()
{
	MultiplexerFactories::iterator mit= multiplexerFactories.find(getArchName());

	if (mit != multiplexerFactories.end())
		multiplexerFactories.erase(getArchName());
}

/**
 * Return Netlet name
 */
std::string CSimpleMultiplexerMetaData::getArchName() const
{
	return archName;
}

/**
 * Create an instance of the Netlet
 */
INetletMultiplexer* CSimpleMultiplexerMetaData::createMultiplexer(CNena *nodeA, IMessageScheduler *sched)
{
	return new CSimpleMultiplexer((IMultiplexerMetaData *) this, nodeA, sched);
}

/* ========================================================================= */

/**
 * @brief	For demo-purposes and tests: instantiate multiple architectures
 */
class CSimpleArchMultiplier
{
public:
	std::list<CSimpleMultiplexerMetaData*> factories;

	CSimpleArchMultiplier()
	{
		list<string>::const_iterator it;
		for (it = simpleArchitectures.begin(); it != simpleArchitectures.end(); it++)
			factories.push_back(new CSimpleMultiplexerMetaData(*it));
	}

	virtual ~CSimpleArchMultiplier()
	{
		std::list<CSimpleMultiplexerMetaData*>::iterator it;
		for (it = factories.begin(); it != factories.end(); it++) {
			delete *it;
		}
		factories.clear();
	}
};

/**
 * Initializer for shared library. Registers factories.
 */
static CSimpleArchMultiplier simpleArchMultiplier;

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
