
#include "netAdaptBoostTap.h"

#include "systemBoost.h"
#include "pcapdef.h"

#include "nena.h"
#include "netAdapt.h"
#include "messages.h"
#include "messageBuffer.h"
#include "debug.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <net/ethernet.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>

#include <string>
#include <cstring>
#include <map>
#include <algorithm>

// Boost libs
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

static const std::string & NETADAPTBOOSTTAP_NAME = "netadapt://edu.kit.tm/itm/tap";

#define TCPIP_ARCH_NAME "architecture://edu.kit.tm/itm/tcpip"

#define TUNTAP_DEVICE "/dev/net/tun"

#define PCAP_MAX_CAPLEN	128

using boost::format;
using boost::shared_ptr;
using namespace std;

/*****************************************************************************/

CBoostTapNetAdapt::CBoostTapNetAdapt (CNena *nodeA, IMessageScheduler *sched,
		const std::string& uri,
		boost::shared_ptr<boost::asio::io_service> ios) :
	INetAdapt(nodeA, sched, std::string(), uri),
	io_service(ios),
	ifdesc(*ios)
{
	className += "::BoostTap"; // would be IMP::INetAdapt::BoostTap
	isReady_ = false;
	setId(NETADAPTBOOSTTAP_NAME);

	string active = nena->getConfigValue(getId(), "active");
	if (active.empty() || (active != "true" && active != "1")) {
		DBG_INFO(FMT("%1%: deactivated in config") % getId());
		return;
	}

	device = nena->getConfigValue(getId(), "device");
	if (device.empty()) {
		device = "nena0";
	}

	setProperty(p_name, new CStringValue(string(getId()) + ":" + device));
	setProperty(p_archid, new CStringValue(TCPIP_ARCH_NAME));
	// This is a wild guess
	setProperty(p_mtu, new CIntValue(1500));

	int ifd = open(TUNTAP_DEVICE, O_RDWR);
	if (ifd < 0) {
		string errstr = strerror(errno);
		DBG_ERROR(FMT("CBoostTapNetAdapt: Error opening %1%: %2% (%3)") % TUNTAP_DEVICE % errstr % errno);
		return;

	}

	struct ifreq ifr;
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, device.c_str(), IFNAMSIZ);
	int err = ioctl(ifd, TUNSETIFF, &ifr);
	if (err != 0) {
		string errstr = strerror(err);
		DBG_ERROR(FMT("CBoostTapNetAdapt: ioctl error %1% on TUNSETIFF: %2%") % err % errstr);
		return;

	}

	// not sure if that's officially supported, but it works ;)
	memset(hwaddr, 0, ETH_ALEN);
	err = ioctl(ifd, SIOCGIFHWADDR, &ifr);
	if (err == 0) {
		memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
		hwaddr_str = (FMT("%02x:%02x:%02x:%02x:%02x:%02x") %
				(unsigned int) hwaddr[0] % (unsigned int) hwaddr[1] % (unsigned int) hwaddr[2] %
				(unsigned int) hwaddr[3] % (unsigned int) hwaddr[4] % (unsigned int) hwaddr[5]).str();

	} else {
		DBG_ERROR(FMT("CBoostTapNetAdapt: ioctl error %1% on SIOCGIFHWADDR") % err);

	}

	DBG_INFO(FMT("%1%: set up on device %2% (%3%)") % getId() % device % hwaddr_str);

	ifdesc.assign(ifd);
//	boost::asio::posix::stream_descriptor::non_blocking_io non_blocking_io_cmd(true);
//	ifdesc.io_control(non_blocking_io_cmd);

	// PCAP file
	string name = nena->getNodeName();
	for (size_t i = 0; i < name.size(); i++) {
		if (name[i] == '/') name[i] = '-';
		if (name[i] == ':') name[i] = '-';
	}

	pcapfile.open((name + "_" + device + ".pcap").c_str());
	assert(pcapfile.good());

	pcap_hdr_t pcaph = {0xa1b2c3d4, 2, 4, 0, 0, PCAP_MAX_CAPLEN, 12 /* DLT_RAW */};
	pcapfile.write(reinterpret_cast<char*>(&pcaph), sizeof(pcaph));

	start_receive();
	isReady_ = true;
}

CBoostTapNetAdapt::~CBoostTapNetAdapt ()
{}

/**
 * @brief	Check whether network adaptor is ready for sending/receiving data
 */
bool CBoostTapNetAdapt::isReady() const
{
	return isReady_;
}

/**
 * @brief callback method for socket
 *
 * We use async receive and call "handle_receive", when something was received.
 *
 */
void CBoostTapNetAdapt::start_receive ()
{
//	ifdesc.async_read_some(boost::asio::null_buffers(),
//			boost::bind (&CBoostTapNetAdapt::handle_receive, this,
//						 boost::asio::placeholders::error,
//						 boost::asio::placeholders::bytes_transferred));
	ifdesc.async_read_some(boost::asio::buffer(recv_buffer, BOOSTTAP_RECVBUFSIZE),
			boost::bind (&CBoostTapNetAdapt::handle_receive, this,
						 boost::asio::placeholders::error,
						 boost::asio::placeholders::bytes_transferred));
}

/// if something is received, this method is called first
void CBoostTapNetAdapt::handle_receive (const boost::system::error_code& error, std::size_t bytes_transferred)
{
	boost::system::error_code err = error;

	if ((bytes_transferred > 0) && (!err || err == boost::asio::error::message_size)) {
		boctet_t* data = reinterpret_cast<boctet_t*>(recv_buffer);

		struct ether_header* ethh = reinterpret_cast<struct ether_header*>(recv_buffer);

		DBG_DEBUG(FMT("CBoostTapNetAdapt: Received tap packet, header(src %02x:%02x:%02x:%02x:%02x:%02x, dst %02x:%02x:%02x:%02x:%02x:%02x, type %d)")
				% (int) ethh->ether_shost[0] % (int) ethh->ether_shost[1] % (int) ethh->ether_shost[2] % (int) ethh->ether_shost[3] % (int) ethh->ether_shost[4] % (int) ethh->ether_shost[5]
				% (int) ethh->ether_dhost[0] % (int) ethh->ether_dhost[1] % (int) ethh->ether_dhost[2] % (int) ethh->ether_dhost[3] % (int) ethh->ether_dhost[4] % (int) ethh->ether_dhost[5]
				% (int) ethh->ether_type
			);

		data = reinterpret_cast<boctet_t*>(&recv_buffer[sizeof(struct ether_header)]);

		// write PCAP record
		double time = nena->getSysTime();
		time += 60 * 60 * 24; // avoid problems due to negative time zones
		uint32_t sec = static_cast<uint32_t>(time);
		uint32_t usec = static_cast<uint32_t>((time - sec) * 1000000);

		pcaprec_hdr_t pcaph = {sec, usec, (bytes_transferred > PCAP_MAX_CAPLEN) ? PCAP_MAX_CAPLEN : bytes_transferred, bytes_transferred};
		pcapfile.write(reinterpret_cast<char*>(&pcaph), sizeof(pcaph));
		pcapfile.write(reinterpret_cast<const char*>(recv_buffer), pcaph.incl_len);
		pcapfile.flush();

		shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(buffer_t(bytes_transferred - sizeof(struct ether_header), data)));
		pkt->setFrom(this);
		pkt->setTo(prev);
		pkt->setType(IMessage::t_incoming);
		pkt->setProperty(IMessage::p_netAdapt, new CPointerValue<INetAdapt>(this));

		sendMessage(pkt);

	} else if (err) {
		DBG_WARNING(FMT("%1%: handle_receive failed with error code %2%") % className % err);

	} else {
		DBG_WARNING(FMT("%1%: ?!? (should not happen)") % className);

	}

	start_receive ();
	return;
}

/// dummy function, called after send
void CBoostTapNetAdapt::handle_send (boost::shared_ptr<CNetworkFrame> frame, const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (error.value() != 0) {
		DBG_ERROR(FMT("Error sending data (%1%): %2%") % error.value() % error.message());

	} else if (bytes_transferred != frame->size) {
		DBG_ERROR(FMT("bytes transferred (%1%) != buffer size (%2%)!") % bytes_transferred % frame->size);

	}
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * Nothing implemented yet.
 *
 * @param msg	Pointer to message
 */
void CBoostTapNetAdapt::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * Nothing implemented yet.
 *
 * @param msg	Pointer to message
 */
void CBoostTapNetAdapt::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * This is the most common case of a application induced message for this NetAdapt.
 *
 * @param msg	Pointer to message
 */
void CBoostTapNetAdapt::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	shared_buffer_t ethh_buf(sizeof(struct ether_header));

	struct ether_header* ethh = reinterpret_cast<struct ether_header*>(ethh_buf.mutable_data());
//	const uint8_t dst[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	const uint8_t* dst = hwaddr; // "gateway"
	const uint8_t src[] = {0x00, 0x0b, 0xad, 0xc0, 0xff, 0xee};
	memcpy(ethh->ether_dhost, dst, ETH_ALEN);
	memcpy(ethh->ether_shost, src, ETH_ALEN);
	ethh->ether_type = htons(ETH_P_IP); // TODO: determine protocol ID

	pkt->push_front(ethh_buf);

	DBG_DEBUG(FMT("CBoostTapNetAdapt: Sending raw packet to %1% (%2% bytes)") % device % pkt->size());

	bsize_t size = pkt->size();
	shared_ptr<CNetworkFrame> frame(new CNetworkFrame(size, msg->getFlowState()));
	pkt->getBuffer().read(frame->buffer);

	// write PCAP record
	double time = nena->getSysTime();
	time += 60 * 60 * 24; // avoid problems due to negative time zones
	uint32_t sec = static_cast<uint32_t>(time);
	uint32_t usec = static_cast<uint32_t>((time - sec) * 1000000);

	pcaprec_hdr_t pcaph = {sec, usec, (size > PCAP_MAX_CAPLEN) ? PCAP_MAX_CAPLEN : size, size};
	pcapfile.write(reinterpret_cast<char*>(&pcaph), sizeof(pcaph));
	pcapfile.write(reinterpret_cast<const char*>(frame->buffer), pcaph.incl_len);
	pcapfile.flush();

	// send frame
	ifdesc.async_write_some(boost::asio::buffer((char*) frame->buffer, frame->size),
			  boost::bind (&CBoostTapNetAdapt::handle_send, this, frame,
						   boost::asio::placeholders::error,
						   boost::asio::placeholders::bytes_transferred));
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CBoostTapNetAdapt::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

const std::string & CBoostTapNetAdapt::getId () const
{
	return NETADAPTBOOSTTAP_NAME;
}

