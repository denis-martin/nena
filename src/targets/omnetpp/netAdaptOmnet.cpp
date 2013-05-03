
#include "nena.h"

#include "netAdaptOmnet.h"
#include "messagesOmnet.h"
#include "systemOmnet.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

#if defined(SYS_OMNET_USE_INET) && defined(SYS_OMNET_USE_MF2)
#error "OMNeT++ target cannot be compiled for INET framework and Mobility Framework at the same time"
#endif

#ifdef SYS_OMNET_USE_INET
#include "IPAddress.h"
#include "IPControlInfo.h"
#include "IInterfaceTable.h"
#include "IPAddressResolver.h"
#include "InterfaceEntry.h"
#include "IPv4InterfaceData.h"
#endif

#ifdef SYS_OMNET_USE_MF2
#include "MacControlInfo.h"
#endif

#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/split.hpp"

#include "boost/shared_ptr.hpp"

#include <string>

const std::string& NETADAPT_OMNET_NAME = "netadapt://edu.kit.tm/itm/omnetpp";

using namespace std;
using boost::shared_ptr;
using edu_kit_tm::itm::simpleArch::CSimpleMultiplexer;

/* ************************************************************************* */

COmnetNetAdapt::COmnetNetAdapt(CNena *nodeA, CSystemOmnet *sys,
		const std::string& arch, const std::string& uri,
		std::string gate, int gateIndex)
	: INetAdapt(nodeA, sys->getMainScheduler(), arch, uri)
{
	className += "::COmnetNetAdapt";

	this->sys = sys;
	this->gate = gate;
	this->gateIndex = gateIndex;
	next = NULL;
	assert(scheduler != NULL);

	outputGate = gate + "$o";
	inputGate = gate + "$i";

	name = str(boost::format("%1%[%2%]") % gate % gateIndex);

	// some test properties
	properties[p_name] = shared_ptr<CMorphableValue>(new CStringValue(name));
	properties[p_up] = shared_ptr<CMorphableValue>(new CBoolValue(true));
	properties[p_linkencap] = shared_ptr<CMorphableValue>(new CStringValue("Ethernet"));
	properties[p_bandwidth] = shared_ptr<CMorphableValue>(new CIntValue(10));
	properties[p_broadcast] = shared_ptr<CMorphableValue>(new CBoolValue(false));
	properties[p_duplex] = shared_ptr<CMorphableValue>(new CBoolValue(true));
	properties[p_mtu] = shared_ptr<CMorphableValue>(new CIntValue(SYS_OMNET_MTU));

	string s = nena->getConfigValue(NETADAPT_OMNET_NAME, "architecture");
	if (s.empty())
		s = SIMPLE_ARCH_NAME; // default architecture
	properties[p_archid] = shared_ptr<CMorphableValue>(new CStringValue(s));

	DBG_INFO(FMT("OmnetNA: COmnetNetAdapt established for gate %1%") % name);
}

COmnetNetAdapt::~COmnetNetAdapt()
{
	prev = NULL;
	properties.clear();
}


/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void COmnetNetAdapt::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void COmnetNetAdapt::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void COmnetNetAdapt::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();

//	DBG_DEBUG(FMT("COmnetNetAdapt: mbuf %1%/%2% (from %3%)")
//			% (int) pkt->getBuffer().length() % pkt->size()
//			% pkt->getFrom()->getClassName());

	COmnetPacket *opkt = NULL;

	if (pkt.get()) {
		opkt = new COmnetPacket(string("NA::" + pkt->getClassName()).c_str());

		// copy message buffers to "network" frame
		opkt->setByteLength(pkt->size());
		opkt->setDataArraySize(pkt->size());
		unsigned int index = 0;
		for (mlength_t bufi = 0; bufi < pkt->getBuffer().length(); bufi++) {
			shared_buffer_t buf = pkt->getBuffer().at(bufi);
			for (bsize_t bytei = 0; bytei < buf.size(); bytei++) {
				opkt->setData(index++, buf[bytei]);
			}
		}
		assert(index == pkt->size());

//		DBG_DEBUG(FMT("OMNET_SEND: %1%") % pkt->to_str());

	} else {
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();

	}

#if defined(SYS_OMNET_USE_INET)

	// add INET IP control info

	if (!iface_addr.isValid()) {
		// get local address
		CSimpleMultiplexer* sm = (CSimpleMultiplexer*) nodeArch->getMultiplexer(edu_kit_tm::itm::simpleArch::SIMPLE_ARCH_NAME);
		iface_addr = sm->getNameAddrMapper()->resolve(nodeArch->getNodeName());
		if (!iface_addr.isValid()) {
			DBG_WARNING("(INET) Cannot resolve local IPv4 locator");
		}

	}

	ipv4::CLocatorValue dstlv;
	try {
		try {
			dstlv = *pkt->getProperty<ipv4::CLocatorValue>(IMessage::p_nextHopLoc);

		} catch(IMessage::EPropertyNotDefined e) {
			dstlv = *pkt->getProperty<ipv4::CLocatorValue>(IMessage::p_destLoc);

		}

		assert(dstlv.isValid());

		IPAddress srcip(iface_addr.getAddr());
		IPAddress dstip(dstlv.getAddr());
		IPControlInfo *ipControlInfo = new IPControlInfo();
		ipControlInfo->setProtocol(IP_PROT_UDP); // abuse of the UDP protocol ID for now
		ipControlInfo->setSrcAddr(srcip);
		ipControlInfo->setDestAddr(dstip);
		ipControlInfo->setInterfaceId(101); // 100 is loopback; TODO: support multiple interfaces
		opkt->setControlInfo(ipControlInfo);

		DBG_INFO(FMT("%1%: Sending packet to %2%") % nodeArch->getNodeName() % dstlv.addrToStr());

		sys->send(opkt, outputGate.c_str(), gateIndex);

	} catch (IMessage::EPropertyNotDefined e) {
		// try multiple locators
		try {
			ipv4::CLocatorList dstll = *pkt->getProperty<ipv4::CLocatorList>(IMessage::p_multiDestLoc);
			list<shared_ptr<ipv4::CLocatorValue> >::iterator it;
			for (it = dstll.list().begin(); it != dstll.list().end(); it++) {
				dstlv = **it;

				COmnetPacket *opkt_n = opkt->dup();

				IPAddress srcip(iface_addr.getAddr());
				IPAddress dstip(dstlv.getAddr());
				IPControlInfo *ipControlInfo = new IPControlInfo();
				ipControlInfo->setProtocol(IP_PROT_UDP); // abuse of the UDP protocol ID for now
				ipControlInfo->setSrcAddr(srcip);
				ipControlInfo->setDestAddr(dstip);
				ipControlInfo->setInterfaceId(101); // 100 is loopback; TODO: support multiple interfaces
				opkt_n->setControlInfo(ipControlInfo);

				DBG_INFO(FMT("%1%: Sending packet to %2%") % nodeArch->getNodeName() % dstlv.addrToStr());

				sys->send(opkt_n, outputGate.c_str(), gateIndex);

			}

			// destroy omnet packte since we duplicated it
			delete opkt;

		} catch (IMessage::EPropertyNotDefined e) {
			throw EUnhandledMessage("(INET) No destination locator defined.");

		}

	}

#elif defined(SYS_OMNET_USE_MF2)

	/*
	 * add MF2 MAC control info
	 *
	 * Mapping between IP addresses and MF2 MAC addresses (which are the same
	 * as the module ID of the respective "nic" module).
	 *
	 */

	ipv4::CLocatorValue dstlv;
	try {

		try {
			dstlv = *pkt->getProperty<ipv4::CLocatorValue>(IMessage::p_nextHopLoc);

		} catch(IMessage::EPropertyNotDefined e) {
			dstlv = *pkt->getProperty<ipv4::CLocatorValue>(IMessage::p_destLoc);

		}

		assert(dstlv.isValid());

		int omnetAddr = dstlv.getAddr() & 0xff;
		int macAddr;
		if (omnetAddr == 255) {
			// broadcast address
			macAddr = -1;

		} else {
			// unicast MAC address
			omnetAddr--;
			cModule *networkMod = sys->getParentModule()->getParentModule();
			assert(networkMod != NULL);
			cModule *hostMod = networkMod->getSubmodule("host", omnetAddr);
			assert(hostMod != NULL);
			macAddr = hostMod->getSubmodule("nic")->getId();

		}

//		DBG_INFO(FMT("%1% (MF2) sending packet to %2% (macAddr %3%)")
//				% nodeArch->getNodeName()
//				% dstlv.addrToStr()
//				% macAddr);

		MacControlInfo* macControlInfo = new MacControlInfo(macAddr);
		opkt->setControlInfo(macControlInfo);

		sys->send(opkt, outputGate.c_str(), gateIndex);

	} catch (IMessage::EPropertyNotDefined e) {
		// try multiple locators
		try {
			cModule *networkMod = sys->getParentModule()->getParentModule();
			assert(networkMod != NULL);

			ipv4::CLocatorList dstll = *pkt->getProperty<ipv4::CLocatorList>(IMessage::p_multiDestLoc);
			list<ipv4::CLocatorValue>::iterator it;
			for (it = dstll.list().begin(); it != dstll.list().end(); it++) {
				dstlv = *it;

				COmnetPacket *opkt_n = opkt->dup();

				int omnetAddr = dstlv.getAddr() & 0xff;
				int macAddr;
				if (omnetAddr == 255) {
					// broadcast address
					macAddr = -1;

				} else {
					// unicast MAC address
					omnetAddr--;
					cModule *hostMod = networkMod->getSubmodule("host", omnetAddr);
					assert(hostMod != NULL);
					macAddr = hostMod->getSubmodule("nic")->getId();

				}

//				DBG_INFO(FMT("%1% (MF2) sending packet to %2% (macAddr %3%)")
//						% nodeArch->getNodeName()
//						% dstlv.addrToStr()
//						% macAddr);

				MacControlInfo* macControlInfo = new MacControlInfo(macAddr);
				opkt_n->setControlInfo(macControlInfo);

				sys->send(opkt_n, outputGate.c_str(), gateIndex);

			}

			// destroy omnet packet since we duplicated it
			delete opkt;

		} catch (IMessage::EPropertyNotDefined e) {
			throw EUnhandledMessage("(MF2) No destination locator defined.");

		}

	}

#else  // !(SYS_OMNET_USE_INET && SYS_OMNET_USE_MF2)

	sys->send(opkt, outputGate.c_str(), gateIndex);

#endif // SYS_OMNET_USE_INET

}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void COmnetNetAdapt::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled incoming message!");
	throw EUnhandledMessage();
}

/**
 * @brief Return a name for this Network Access
 */
const std::string& COmnetNetAdapt::getName() const
{
	return name;
}

const std::string& COmnetNetAdapt::getId() const
{
	return NETADAPT_OMNET_NAME;
}

/**
 * Invoked when a packet is received via omnet
 */
void COmnetNetAdapt::handleOmnetPacket(COmnetPacket *opkt)
{
	assert(prev != NULL);
	assert(opkt->getDataArraySize() > 0);

	shared_ptr<CMessageBuffer> mbuf(new CMessageBuffer(this, prev, IMessage::t_incoming, opkt->getDataArraySize()));
//	DBG_INFO(FMT("Received OMNET packet of size %1%, buffer size allocated: %2%") %
//			opkt->getDataArraySize() % mbuf->size());
	for (unsigned int i = 0; i < opkt->getDataArraySize(); i++) {
		mbuf->getBuffer().write<char>(opkt->getData(i), i);
	}

//	DBG_DEBUG(FMT("OMNET_RECV: %1%") % mbuf->to_str());

	mbuf->setProperty(IMessage::p_netAdapt, new CPointerValue<INetAdapt>(this));
	sendMessage(mbuf);

	// opkt is deleted in CSystemOmnet::handleMessage()
}

/**
 * Configure interfaces (e.g. IP addresses)
 *
 * Note, that this assumes a compound module structure as, for instance,
 * used in examples/omnet/wireless01.
 */
void COmnetNetAdapt::configureInterface()
{
#ifdef SYS_OMNET_USE_INET
	/*
	 * Extract the IP address from resolver.conf
	 *
	 * This part was taken from CSimpleNameAddrMapper.
	 */

	map<string, ipv4::CLocatorValue> resolveMap;
	ifstream is("resolver.conf");
	if (!is.good()) {
		DBG_WARNING("COmnetNetAdapt: WARNING: Cannot open \"resolver.conf\". "
			"Assuming dynamic name resolution... "
			"If in doubt, please read the README file.");
		return;
	}

	int line = 0;
	string s;
	const string whitespaces = " \t\r\n";
	while (!is.eof()) {
		line++;
		getline(is, s);

		// trim
		boost::trim(s);
		// check for comment
		if (s[0] != '#') {
			vector<string> sl;
			boost::split(sl, s, boost::is_any_of(whitespaces), boost::token_compress_on);
			if (sl.size() > 0 && sl[0] != "") {
				if (sl.size() != 2) {
					DBG_ERROR(FMT("COmnetNetAdapt: resolver.conf:%1%: format error") % line);

				} else {
					ipv4::CLocatorValue lv(sl[1]);
					if (!lv.isValid()) {
						DBG_ERROR(FMT("COmnetNetAdapt: resolver.conf:%1%: format error (in IP address)") % line);

					} else {
						if (sl[0] == "") {
							DBG_ERROR(FMT("COmnetNetAdapt: resolver.conf:%1%: format error (in node name)") % line);

						} else {
							resolveMap[sl[0]] = lv;

						}
					}
				}
			}
		}
	}

	/*
	 * Set the IP address of the INet interface to the one found in
	 * resolver.conf, and the netmask to 255.255.255.0.
	 */

	cModule *parentMod = sys->getParentModule();
	if (parentMod == NULL || IPAddressResolver().findInterfaceTableOf(parentMod) == NULL) {
		DBG_WARNING("INet framework support is enabled, but unabled to find interface table.\n"
			"Please use the same structure as, for instance, examples/omnet/wireless01.");
		return;
	}

	// set IP address on every interface
	if (!resolveMap[sys->getNodeName()].isValid()) {
		DBG_WARNING("INet framework support is enabled, but unabled to resolve own address.\n"
			"Please use the same configuration as, for instance, examples/omnet/wireless01.");
		return;
	}
	IPAddress myAddress(resolveMap[sys->getNodeName()].addrToStr().c_str());

	IInterfaceTable* ift = IPAddressResolver().interfaceTableOf(parentMod);
	for (int k = 0; k < ift->getNumInterfaces(); k++)
	{
		InterfaceEntry *ie = ift->getInterface(k);
		if (!ie->isLoopback())
		{
			ie->ipv4Data()->setIPAddress(myAddress);
			ie->ipv4Data()->setNetmask(IPAddress(0xffffff00)); // currently assuming a /24 network
		}
	}

#endif
}
