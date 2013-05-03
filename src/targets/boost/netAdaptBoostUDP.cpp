
#include "netAdaptBoostUDP.h"

#include "systemBoost.h"

#include "nena.h"
#include "debug.h"

#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <pugixml.h>

#define NETADAPTBOOSTUDP_NAME "netadapt://boost/udp/"

using boost::format;
using boost::asio::ip::udp;
using boost::shared_ptr;
using namespace ipv4;
using namespace std;

/*****************************************************************************/

CBoostUDPNetAdapt::CBoostUDPNetAdapt(CNena *nodeA, IMessageScheduler *sched,
		const std::string& uri,
		boost::shared_ptr<boost::asio::io_service> ios) :
	CBoostNetAdapt(nodeA, sched, uri, ios),
	socket(*ios)
{
	className += "::CBoostUDPNetAdapt";

	if (getId().empty()) {
		setId(NETADAPTBOOSTUDP_NAME);
	}

	vector<string> hosts;
	try {
		nena->getConfig()->getParameterList(NETADAPTBOOSTUDP_NAME, "allowedHosts", hosts);

		for (vector<string>::const_iterator it = hosts.begin (); it != hosts.end (); it++) {
			allowedHosts[ipv4::CLocatorValue(*it + ":0").getAddr()] = true;
		}

	} catch (...) {};

	// get config

	bool configIsGood = true;
	string ip = "0.0.0.0";
	uint32_t port = 50779;

	if (nodeA->getConfig()->hasParameter(getId(), "ip", XMLFile::STRING, XMLFile::VALUE)) {
		nodeA->getConfig()->getParameter(getId(), "ip", ip);
		// TODO check validity

	}

	if (nodeA->getConfig()->hasParameter(getId(), "port", XMLFile::UINT32_T, XMLFile::VALUE)) {
		nodeA->getConfig()->getParameter(getId(), "port", port);
		if (port < 1024 || port > 0xffff) {
			DBG_ERROR(FMT("%1%: Cannot set port to %2%") % getId() % port);
			configIsGood = false;

		}

	}

	if (nodeA->getConfig()->hasParameter(getId(), "arch", XMLFile::STRING, XMLFile::VALUE)) {
		string s;
		nodeA->getConfig()->getParameter(getId(), "arch", s);
		if (!s.empty()) {
			setArchId(s);
			setProperty(p_archid, new CStringValue(getArchId()));

		} else {
			DBG_ERROR(FMT("%1%: Configuration incomplete: missing architecture name") % getId());
			configIsGood = false;

		}

	} else {
		DBG_ERROR(FMT("%1%: Configuration incomplete: missing architecture name") % getId());
		configIsGood = false;

	}

	if (nodeA->getConfig()->hasParameter(getId(), "maxbps", XMLFile::UINT32_T, XMLFile::VALUE)) {
		uint32_t maxbps = 0;
		nodeA->getConfig()->getParameter(getId(), "maxbps", maxbps);
		setProperty(p_bandwidth, new CIntValue(maxbps));

	}

	if (nodeA->getConfig()->hasParameter(getId(), "broadcast", XMLFile::STRING, XMLFile::LIST)) {
		shared_ptr<ipv4::CLocatorList> broadcastList(new ipv4::CLocatorList());

		vector<string> broadcastAddr;
		nodeA->getConfig()->getParameterList(getId(), "broadcast", broadcastAddr);
		vector<string>::const_iterator it;
		for (it = broadcastAddr.begin(); it != broadcastAddr.end(); it++) {
			shared_ptr<ipv4::CLocatorValue> loc(new ipv4::CLocatorValue(*it));
			broadcastList->list().push_back(loc);
		}
		setProperty(p_broadcast, broadcastList);

	}

	if (nodeA->getConfig()->hasParameter(getId(), "droprate", XMLFile::FLOAT, XMLFile::VALUE)) {
		nodeA->getConfig()->getParameter(getId(), "droprate", dropRate);
		DBG_DEBUG(FMT("%1%: setting drop rate to %2%") % getId() % dropRate);

	}

	if (configIsGood) {
		setProperty(p_addr, new CStringValue((FMT("%1%:%2%") % ip % port).str()));
		setConfig();

	}
}

CBoostUDPNetAdapt::~CBoostUDPNetAdapt ()
{}

/**
 * @brief	Check whether network adaptor is ready for sending/receiving data
 */
bool CBoostUDPNetAdapt::isReady() const
{
	return socket.is_open();
}

/**
 * @brief	Start async receive
 *
 * 			When something is received, handle_receive() will be called by boost's io_service.
 */
void CBoostUDPNetAdapt::start_receive()
{
	assert(recv_buffer.use_count() <= 2); // pool plus recv_buffer variable

	socket.async_receive_from (boost::asio::buffer((char*) ((buffer_t) recv_buffer).mutable_data(), BOOST_RECVBUFSIZE), sender,
		boost::bind (&CBoostUDPNetAdapt::handle_receive, this,
					 boost::asio::placeholders::error,
					 boost::asio::placeholders::bytes_transferred));
}

/**
 * @brief	Additional packet mangling of child classes (optional)
 */
bool CBoostUDPNetAdapt::mangle_incoming(boost::shared_ptr<CMessageBuffer>& pkt)
{
	unsigned long v4addr = sender.address().to_v4().to_ulong();
	// check sender first (drop all packets not coming from an allowed host)
	if (!allowedHosts.empty()) {
		map<uint32_t, bool>::iterator isAllowed = allowedHosts.find(v4addr);
		if ((isAllowed == allowedHosts.end()) || !(isAllowed->second)) {
			// host not found or disabled
			DBG_WARNING(FMT("%1%: Received packet from disallowed host %2%") %
					getId() % sender.address().to_string());

		} else {
			return true;

		}

	} else {
		DBG_WARNING(FMT("%1%: no allowed hosts found. Dropping packet from %2%:%3%...") %
				getId() % sender.address().to_string() % sender.port());

	}

	return false;
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * This is the most common case of a application induced message for this NetAdapt.
 *
 * @param msg	Pointer to message
 */
void CBoostUDPNetAdapt::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	bsize_t size = pkt->size();
	shared_ptr<CNetworkFrame> frame(new CNetworkFrame(size, msg->getFlowState()));
	pkt->getBuffer().read(frame->buffer);

	// determine target
	list<shared_ptr<CLocatorValue> > ll;

	try {
		shared_ptr<CLocatorValue> lv = pkt->getProperty<CLocatorValue>(IMessage::p_nextHopLoc);
//		DBG_INFO(FMT("Target address: %1%:%2%") % lv->addrToStr() % lv->getPort());
		ll.push_front(lv);

	} catch (IMessage::EPropertyNotDefined& e) {
		try {
			shared_ptr<CLocatorValue> lv = pkt->getProperty<CLocatorValue>(IMessage::p_destLoc);
//			DBG_INFO(FMT("Target address: %1%:%2%") % lv.addrToStr() % lv.getPort());
			ll.push_front(lv);

		} catch (IMessage::EPropertyNotDefined& e) {
			// try multi locator property
			ll = pkt->getProperty<CLocatorList>(IMessage::p_multiDestLoc)->list();

		}

	}

	for (list<shared_ptr<CLocatorValue> >::const_iterator it = ll.begin(); it != ll.end(); it++) {
		shared_ptr<CLocatorValue> lv = *it;
//		DBG_INFO(FMT("Target address: %1%:%2%") % lv.addrToStr() % lv.getPort());
		udp::endpoint target = udp::endpoint(
			boost::asio::ip::address::from_string(lv->addrToStr()), lv->getPort());

		if (dropRate == 0 || nena->getSys()->random() > dropRate) {
			socket.async_send_to (boost::asio::buffer((char*) frame->buffer, frame->size), target,
								  boost::bind (&CBoostUDPNetAdapt::handle_send, this, frame,
											   boost::asio::placeholders::error,
											   boost::asio::placeholders::bytes_transferred));

		} else {
			if (frame->flowState != NULL)
				FLOWSTATE_FLOATOUT_DEC(frame->flowState, 1, "net", nena->getSysTime());

		}

	}
}

/**
 * @grief	Return architecture-specific locator of last sender
 */
boost::shared_ptr<CMorphableValue> CBoostUDPNetAdapt::getLastSender()
{
	shared_ptr<ipv4::CLocatorValue> s(new ipv4::CLocatorValue(sender.address().to_v4().to_ulong(), sender.port()));
	return s;
}

/**
 * @brief	Manually set configuration. Socket is bound to port and
 * 			architecture given in the netadapt's property list.
 */
void CBoostUDPNetAdapt::setConfig()
{
	if (socket.is_open()) {
		DBG_ERROR(FMT("%1%: Socket already bound, cannot set configuration") % getId());

	} else {
		string addr;
		try {
			addr = getProperty<CStringValue>(p_addr)->value();

		} catch (...) {
			DBG_ERROR(FMT("%1%: No address defined") % getId());
			return;

		}

		size_t col = addr.find(':');
		string ip = addr.substr(0, col);
		uint32_t port = atoi(addr.substr(col+1).c_str());

		// auto-generate ID if necessary
		if (getId() == NETADAPTBOOSTUDP_NAME) {
			setId((FMT("%1%%2%") % NETADAPTBOOSTUDP_NAME % addr).str());
		}

		setProperty(p_name, new CStringValue(getId()));

		socket.open(udp::v4());
		socket.bind(udp::endpoint(boost::asio::ip::address::from_string(ip), port));

		socket.set_option(udp::socket::broadcast(true));
		socket.set_option(udp::socket::reuse_address(true));

		/*
		 * Since we receive a lot of packets per video frame (~200/300kB), we need
		 * to increase the socket's receive buffer size. This may be limited by the
		 * OS (Linux: 131071 bytes). To increase the maximum receive buffer size
		 * for UDP sockets under Linux use the following command:
		 *
		 *   sudo sysctl -w net.core.rmem_max=2097152
		 *
		 * This sets the maximum allowed buffer size to 2 MB.
		 *
		 * Similarly, we set the maximum allowed send buffer size to 1 MB to allow
		 * for burst sent packets:
		 *
		 *   sudo sysctl -w net.core.wmem_max=1048576
		 */
	//	socket.set_option(boost::asio::socket_base::receive_buffer_size(BOOST_SYSRECVBUFSIZE));
	//	socket.set_option(boost::asio::socket_base::send_buffer_size(BOOST_SYSSENDBUFSIZE));

		start_receive();

	}
}
