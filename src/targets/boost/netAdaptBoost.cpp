
#include "netAdaptBoost.h"

#include "systemBoost.h"

#include "nena.h"
#include "debug.h"

#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <pugixml.h>
#include <sstream>

#define NETADAPTBOOST_NAME	"netadapt://boost/"

/// measuring bandwidth
#define BOOST_STAT_INTERVAL	1

using boost::format;
using boost::asio::ip::udp;
using boost::shared_ptr;
using namespace std;
using namespace pugi;

/*****************************************************************************/

CBoostNetAdapt::CBoostNetAdapt(CNena *nodeA, IMessageScheduler *sched,
		const std::string& uri,
		boost::shared_ptr<boost::asio::io_service> ios) :
	INetAdapt(nodeA, sched, std::string(), uri),
	io_service(ios),
	dropRate(0),
	recvBufferPool(BOOST_RECVBUFSIZE)
{
	className += "::CBoostNetAdapt"; // would be IMP::INetAdapt::CBoostNetAdapt

	setProperty(p_mtu, new CIntValue(1400)); // This is a wild guess
	setProperty(p_tx_rate, new CIntValue(0));
	setProperty(p_rx_rate, new CIntValue(0));
	setProperty(p_bandwidth, new CIntValue(0));
	setProperty(p_freeBandwidth, new CIntValue(0));

	recv_buffer = recvBufferPool.get();

	stat_rx_bytes = 0;
	stat_tx_bytes = 0;

#ifdef BOOST_STAT_INTERVAL
	scheduler->setTimer(new CTimer(BOOST_STAT_INTERVAL, this));
#endif
}

CBoostNetAdapt::~CBoostNetAdapt()
{}

/**
 * @brief	Callback after something has been received
 */
void CBoostNetAdapt::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if ((bytes_transferred > 0) && (!error || error == boost::asio::error::message_size)) {
		stat_rx_bytes += bytes_transferred;

		if (prev != NULL) {
			// -g costs around 25 MB/s (the following 8 lines)
			shared_ptr<CMessageBuffer> pkt = messageBufferPool.get();
			pkt->push_back(recv_buffer(0, bytes_transferred)); // sub buffer
			pkt->setFrom(this);
			pkt->setTo(prev);
			pkt->setType(IMessage::t_incoming);

			recv_buffer.reset(); // drop reference
			recv_buffer = recvBufferPool.get(); // new receive buffer

			// -g costs around 27 MB/s (the following two lines!) - could be optimized with flow state object
			pkt->setProperty(IMessage::p_srcLoc, getLastSender());
			pkt->setProperty(IMessage::p_netAdapt, new CPointerValue<INetAdapt>(this));

			if (mangle_incoming(pkt)) // in case some additional mangling is necessary
				sendMessage(pkt); // flow state is determined later -> no chance to adapt floating packets here

		}

	} else if (error) {
		DBG_WARNING(FMT("%1%: handle_receive failed with error code %2%") % getId() % error);

	}

	start_receive();
}

/**
 * @brief	Callback after a send operation
 */
void CBoostNetAdapt::handle_send(boost::shared_ptr<CNetworkFrame> frame, const boost::system::error_code& error, std::size_t bytes_transferred)
{
	stat_tx_bytes += bytes_transferred;

	if (frame->flowState.get()) { // is NULL for relayed packets and control packets w/o flow states
		FLOWSTATE_FLOATOUT_DEC(frame->flowState, 1, "net", nena->getSysTime());
		frame->flowState->notify(this, CFlowState::ev_flowControl);
	}

	if (bytes_transferred != frame->size) {
		DBG_ERROR(FMT("%1%: bytes transferred (%2%) != buffer size (%3%)!") % getId() % bytes_transferred % frame->size);
	}

	if (error.value() != 0) {
		DBG_ERROR(FMT("%1%: error sending data (%2%): %3%") % getId() % error.value() % error.message());
	}
}

/**
 * @brief	Additional packet mangling of child classes (optional)
 */
bool CBoostNetAdapt::mangle_incoming(boost::shared_ptr<CMessageBuffer>& pkt)
{
	// always accept by default
	return true;
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * Nothing implemented yet.
 *
 * @param msg	Pointer to message
 */
void CBoostNetAdapt::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CBoostNetAdapt::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
#ifdef BOOST_STAT_INTERVAL
	// we only have one timer until now
	shared_ptr<CIntValue> rx_rate = getProperty(p_rx_rate)->cast<CIntValue>();
	shared_ptr<CIntValue> tx_rate = getProperty(p_tx_rate)->cast<CIntValue>();

	size_t rxrate = stat_rx_bytes / BOOST_STAT_INTERVAL;
	size_t txrate = stat_tx_bytes / BOOST_STAT_INTERVAL;

	rx_rate->set(rxrate);
	tx_rate->set(txrate);

	stat_rx_bytes = 0;
	stat_tx_bytes = 0;
	scheduler->setTimer(new CTimer(BOOST_STAT_INTERVAL, this));

//	DBG_INFO(FMT("STAT %1% tx_rate %2% rx_rate %3%") % getId() % txrate % rxrate);
#endif // BOOST_STAT_INTERVAL
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CBoostNetAdapt::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}
