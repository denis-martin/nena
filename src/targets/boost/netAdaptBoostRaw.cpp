
#include "netAdaptBoostRaw.h"

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

static const std::string & NETADAPTBOOSTRAW_NAME = "netadapt://edu.kit.tm/itm/raw";

#define PCAP_MAX_CAPLEN	128

using boost::format;
using boost::shared_ptr;
using namespace std;

/*****************************************************************************/

CBoostRawNetAdapt::CBoostRawNetAdapt (CNena *nodeA, IMessageScheduler *sched,
		const std::string& uri,
		boost::shared_ptr<boost::asio::io_service> ios) :
	INetAdapt(nodeA, sched, std::string(), uri),
	io_service(ios),
	ifdesc(*ios)
{
	className += "::BoostRaw"; // would be IMP::INetAdapt::BoostRaw
	isReady_ = false;
	setId(uri);
	setProperty(p_name, new CStringValue(getId()));

	if (nena->getConfig()->hasParameter(getId(), "arch", XMLFile::STRING, XMLFile::VALUE)) {
		string arch;
		nena->getConfig()->getParameter(getId(), "arch", arch);
		setArchId(arch);
		setProperty(p_archid, new CStringValue(getArchId()));

	} else {
		DBG_ERROR(FMT("%1%: Configuration incomplete: missing architecture name (arch)") % getId());
		return;

	}

	// TODO make this configurable
	ethertype = htons(ETHERTYPE_IPV6);

	int ifd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (ifd < 0) {
		string errstr = strerror(errno);
		DBG_ERROR(FMT("%1%: Error opening packet socket: %2% (%3%)") % getId() % errstr % errno);
		return;

	}

	ifindex = 0;
	if (nena->getConfig()->hasParameter(getId(), "interface", XMLFile::STRING, XMLFile::VALUE)) {
		nena->getConfig()->getParameter(getId(), "interface", interface);

		struct ifreq ifr;

		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, interface.c_str(), sizeof(ifr.ifr_name));
		if (ioctl(ifd, SIOCGIFINDEX, &ifr) < 0) {
			string errstr = strerror(errno);
			DBG_ERROR(FMT("%1%: ioctl error getting interface index for %2%: %3% (%4%)") % getId() % interface % errstr % errno);
			close(ifd);
			return;

		}
		ifindex = ifr.ifr_ifindex;

		if (setsockopt(ifd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
			string errstr = strerror(errno);
			DBG_ERROR(FMT("%1%: Cannot bind to device %2%: %3% (%4%)") % getId() % interface % errstr % errno);
			close(ifd);
			return;
		}

		if (ioctl(ifd, SIOCGIFHWADDR, &ifr) < 0) {
			string errstr = strerror(errno);
			DBG_ERROR(FMT("%1%: ioctl error getting hwaddr for %2%: %3% (%4%)") % getId() % interface % errstr % errno);
			close(ifd);
			return;

		}

		memcpy(&hwaddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
		hwaddr_str = (FMT("%02x:%02x:%02x:%02x:%02x:%02x") %
				(int) (unsigned char) hwaddr.sa_data[0] %
				(int) (unsigned char) hwaddr.sa_data[1] %
				(int) (unsigned char) hwaddr.sa_data[2] %
				(int) (unsigned char) hwaddr.sa_data[3] %
				(int) (unsigned char) hwaddr.sa_data[4] %
				(int) (unsigned char) hwaddr.sa_data[5]).str();
		DBG_INFO(FMT("%1%: hwaddr is %2%") % getId() % hwaddr_str);


	} else {
		DBG_ERROR(FMT("%1%: Configuration incomplete: missing interface") % getId());
		close(ifd);
		return;

	}

	// This is a wild guess
	setProperty(p_mtu, new CIntValue(1500));

	DBG_INFO(FMT("%1%: set up on ifindex %2% (%3%)") % getId() % ifindex % interface);

	ifdesc.assign(ifd);
//	boost::asio::posix::stream_descriptor::non_blocking_io non_blocking_io_cmd(true);
//	ifdesc.io_control(non_blocking_io_cmd);

	// PCAP file
	string name = nena->getNodeName();
	for (size_t i = 0; i < name.size(); i++) {
		if (name[i] == '/') name[i] = '-';
		if (name[i] == ':') name[i] = '-';
	}

	pcapfile.open((FMT("%1%_%2%.pcap") % name % interface).str().c_str());
	assert(pcapfile.good());

	pcap_hdr_t pcaph = {0xa1b2c3d4, 2, 4, 0, 0, PCAP_MAX_CAPLEN, DLT_EN10MB};
	pcapfile.write(reinterpret_cast<char*>(&pcaph), sizeof(pcaph));

	start_receive();
	isReady_ = true;
}

CBoostRawNetAdapt::~CBoostRawNetAdapt ()
{}

/**
 * @brief	Check whether network adaptor is ready for sending/receiving data
 */
bool CBoostRawNetAdapt::isReady() const
{
	return isReady_;
}

/**
 * @brief callback method for socket
 *
 * We use async receive and call "handle_receive", when something was received.
 *
 */
void CBoostRawNetAdapt::start_receive ()
{
//	ifdesc.async_read_some(boost::asio::null_buffers(),
//			boost::bind (&CBoostRawNetAdapt::handle_receive, this,
//						 boost::asio::placeholders::error,
//						 boost::asio::placeholders::bytes_transferred));
	ifdesc.async_read_some(boost::asio::buffer(recv_buffer, BOOSTRAW_RECVBUFSIZE),
			boost::bind (&CBoostRawNetAdapt::handle_receive, this,
						 boost::asio::placeholders::error,
						 boost::asio::placeholders::bytes_transferred));
}

/// if something is received, this method is called first
void CBoostRawNetAdapt::handle_receive (const boost::system::error_code& error, std::size_t bytes_transferred)
{
	boost::system::error_code err = error;

	if ((bytes_transferred > 0) && (!err || err == boost::asio::error::message_size)) {
		boctet_t* data = reinterpret_cast<boctet_t*>(recv_buffer);

		struct ether_header* ethh = reinterpret_cast<struct ether_header*>(recv_buffer);

		if (ethh->ether_type == ethertype) {
//			DBG_DEBUG(FMT("CBoostRawNetAdapt: Received raw packet, header(src %02x:%02x:%02x:%02x:%02x:%02x, dst %02x:%02x:%02x:%02x:%02x:%02x, type %d)")
//					% (int) ethh->ether_shost[0] % (int) ethh->ether_shost[1] % (int) ethh->ether_shost[2] % (int) ethh->ether_shost[3] % (int) ethh->ether_shost[4] % (int) ethh->ether_shost[5]
//					% (int) ethh->ether_dhost[0] % (int) ethh->ether_dhost[1] % (int) ethh->ether_dhost[2] % (int) ethh->ether_dhost[3] % (int) ethh->ether_dhost[4] % (int) ethh->ether_dhost[5]
//					% (int) ethh->ether_type
//				);

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

		}

	} else if (err) {
		DBG_WARNING(FMT("%1%: handle_receive failed with error code %2%") % className % err);

	} else {
		DBG_WARNING(FMT("%1%: ?!? (should not happen)") % className);

	}

	start_receive ();
	return;
}

/// dummy function, called after send
void CBoostRawNetAdapt::handle_send (boost::shared_ptr<CNetworkFrame> frame, const boost::system::error_code& error, std::size_t bytes_transferred)
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
void CBoostRawNetAdapt::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
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
void CBoostRawNetAdapt::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
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
void CBoostRawNetAdapt::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	shared_buffer_t ethh_buf(sizeof(struct ether_header));

	struct ether_header* ethh = reinterpret_cast<struct ether_header*>(ethh_buf.mutable_data());
	const uint8_t dst[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; // TODO get dst addr from mbuf properties
	memcpy(ethh->ether_dhost, dst, ETH_ALEN);
	memcpy(ethh->ether_shost, hwaddr.sa_data, ETH_ALEN);
	ethh->ether_type = ethertype;

	pkt->push_front(ethh_buf);

	DBG_DEBUG(FMT("CBoostRawNetAdapt: Sending raw packet to %1% (%2% bytes)") % interface % pkt->size());

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

	DBG_DEBUG("CBoostRawNetAdapt: Sending disabled");

/*
	// send frame
	ifdesc.async_write_some(boost::asio::buffer((char*) frame->buffer, frame->size),
			  boost::bind (&CBoostRawNetAdapt::handle_send, this, frame,
						   boost::asio::placeholders::error,
						   boost::asio::placeholders::bytes_transferred));
*/
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CBoostRawNetAdapt::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

