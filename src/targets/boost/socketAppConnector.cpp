#include "socketAppConnector.h"

#include "systemBoost.h"
#include "nena.h"
#include "messages.h"
#include "messageBuffer.h"
#include "debug.h"
#include "msg.h"
#include "enhancedAppConnector.h"
#include "netletSelector.h"

#include <cstring>

#include <boost/asio/error.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

#define BOOSTSOCK_DEFAULT_SOCKET	"/tmp/nena_socket_default"

/// after tests, 128K is better (max throughput)
#define BOOSTSOCK_SYSRECVBUFSIZE	131072		///< local socket receive buffer size
#define BOOSTSOCK_SYSSENDBUFSIZE	131072		///< local socket send buffer size
#define BOOSTSOCK_QUEUETOAPP_SIZE	(24*10)

using boost::asio::local::stream_protocol;
using boost::shared_ptr;
using boost::scoped_ptr;
using boost::unique_lock;
using boost::mutex;

using namespace std;

static const string & connectorName = "appConnector://boost/socket/";
static const string & serverName = "appServer://boost/socket/";

CSocketAppConnector::CSocketAppConnector(CNena * nodearch, IMessageScheduler * sched,
		boost::shared_ptr<boost::asio::io_service> ios, IAppServer * server) :
				IEnhancedAppConnector(sched, nodearch, server),
				s(*ios),
				buffer((std::size_t) 0),
				writeToAppInProgress(false),
				readFromAppInProgress(false),
				releaseOnSendComplete(false),
				io_service(ios)
{
	className += "::CSocketAppConnector";
	setId(connectorName);

//	DBG_INFO(boost::format("%1%: born %2$p (scheduler %3$p)") % getId() % this % scheduler);
}

CSocketAppConnector::~CSocketAppConnector()
{
	s.close();
}

void CSocketAppConnector::start()
{
	boost::asio::socket_base::receive_buffer_size sopt_rbuf(BOOSTSOCK_SYSRECVBUFSIZE);
	boost::asio::socket_base::send_buffer_size sopt_sbuf(BOOSTSOCK_SYSSENDBUFSIZE);

	s.set_option(sopt_rbuf);
	s.set_option(sopt_sbuf);

	s.get_option(sopt_rbuf);
	s.get_option(sopt_sbuf);

	/// get data
	{
		unique_lock<mutex> blockingVariables;
		readFromAppInProgress = true;
		start_header_receive();
	}

	DBG_INFO(FMT("%1%: Waiting for next messages (rbuf %2%, sbuf %3%)...") % getId() % sopt_rbuf.value() % sopt_sbuf.value());

}

void CSocketAppConnector::stop()
{
	s.cancel();
	DBG_DEBUG(FMT("%1%: stopped (%2%)") % getId() % getRemoteURI());
}

void CSocketAppConnector::sendPayload(MSG_TYPE type, shared_buffer_t payload)
{
	uint32_t t = static_cast<uint32_t>(type);
	uint32_t size = payload.size();
	shared_buffer_t data(2 * sizeof(uint32_t) + size);

	*reinterpret_cast<uint32_t*>(&data.mutable_data()[0]) = t;
	*reinterpret_cast<uint32_t*>(&data.mutable_data()[sizeof(uint32_t)]) = size;
	memcpy(&data.mutable_data()[2 * sizeof(uint32_t)], payload.data(), size);

	boost::asio::async_write(s, boost::asio::buffer(data.data(), data.size()), boost::asio::transfer_all(),
			boost::bind(&CSocketAppConnector::send_complete, this, data, boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

}

/**
 * @brief cleanup fkt. for send operation
 *
 * @param data string with payload
 */
void CSocketAppConnector::send_complete(shared_buffer_t data, const boost::system::error_code& error,
		std::size_t bytes_transferred)
{
	if (error) {
		DBG_ERROR(
				FMT("%1%: %2% failed on send with: %3%") % getId() % (identifier.empty () ? "<unassigned>" : identifier) % error.message());
	}

	if (releaseOnSendComplete)
		release();
}

/**
 * @brief receives header of incoming messages
 */
void CSocketAppConnector::handle_header(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (!error) {
//		DBG_DEBUG(FMT("AppConnector %1% got Header, Type: %2% Length: %3%.") % (identifier.empty () ? "<unassigned>" : identifier) % header[0] % header[1] );

		if (header[1] > (1 << 24)) // for debugging: limit data chunk to 16 MB
			DBG_FAIL(FMT("Data chunk too big (%1% bytes)") % header[1]);
		//if (header[1] == 0)
		//DBG_FAIL(FMT("%1%: received message of length 0") % className);

		buffer = shared_buffer_t(header[1]);
		//memset(buffer, 0, header[1]);

		bytes_left = header[1];

		boost::asio::async_read(s, boost::asio::buffer(buffer.mutable_data(), header[1]), boost::asio::transfer_all(),
				boost::bind(&CSocketAppConnector::handle_body, this, boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));

	} else {
		if (error != boost::asio::error::eof) {
			DBG_ERROR(
					FMT("%1%: %2% failed on Header receive (%3%: %4%).") % getId() % (identifier.empty () ? "unassigned" : identifier) % error.value() % error.message());
		}

		if ((getFlowState() != NULL) && (getFlowState()->getOperationalState() == CFlowState::s_valid))
			getFlowState()->setOperationalState(this, CFlowState::s_stale);
		release(); // die

	}
}

/**
 * @brief receives body of incoming messages and processes them
 */
void CSocketAppConnector::handle_body(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	bytes_left -= bytes_transferred;

	if (!error) {
		if (bytes_left > 0)
			DBG_FAIL(
					FMT("%1%: %2% received %3% bytes but %4% bytes missing!") % className % (identifier.empty () ? "unassigned" : identifier) % (header[1]-bytes_left) % bytes_left);

//		DBG_DEBUG(FMT("AppConnector %1% handling payload...") % (identifier.empty () ? "<unassigned>" : identifier));

		handlePayload(static_cast<MSG_TYPE>(header[0]), buffer);

		buffer.reset(); // drop reference

		unique_lock<mutex> blockingVariables;
		if (!appRecvBlocked && (header[0] != MSG_TYPE_END)) {
			readFromAppInProgress = true;
			start_header_receive();

		} else {
			// blocked
//			DBG_DEBUG(FMT("%1%: blocked or ended") % getId());
			readFromAppInProgress = false;

		}

	} else {
		switch (error.value()) {
		case boost::asio::error::eof: {
			// not sure if this can happen
			if (bytes_left > 0 && bytes_transferred > 0) {
				DBG_ERROR(
						FMT("%1%: %2% error code: %3%, received only: %4%, still remaining: %5%.") % className % identifier % error % bytes_transferred % bytes_left);

				boost::asio::async_read(s,
						boost::asio::buffer(buffer.mutable_data() + (header[1] - bytes_left), bytes_left),
						boost::asio::transfer_all(),
						boost::bind(&CSocketAppConnector::handle_body, this, boost::asio::placeholders::error,
								boost::asio::placeholders::bytes_transferred));
			} else {
				DBG_ERROR(
						FMT("%1%: %2% failed on Body receive with error code: %3%, received: %4%.") % className % identifier % error % bytes_transferred);
				if (getFlowState() != NULL)
					getFlowState()->setOperationalState(this, CFlowState::s_stale);
				release(); // die
			}

			break;
		}
		default: {
			DBG_ERROR(
					FMT("%1%: %2% failed on Body receive with error code: %3%, received: %4%.") % className % identifier % error, bytes_transferred);
			if (getFlowState() != NULL)
				getFlowState()->setOperationalState(this, CFlowState::s_stale);
			release(); // die
			break;
		}
		}
	}
}

void CSocketAppConnector::start_header_receive()
{
	boost::asio::async_read(s, boost::asio::buffer(reinterpret_cast<char *>(header), 2 * sizeof(unsigned int)),
			boost::asio::transfer_all(),
			boost::bind(&CSocketAppConnector::handle_header, this, boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * Nothing implemented yet.
 *
 * @param msg	Pointer to message
 */
void CSocketAppConnector::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> ev = msg->cast<IEvent>();
	assert(ev.get());

	if (ev->getId() == FLOWSTATE_NOTIFICATION_EVENT) {
		shared_ptr<CFlowState::Notification> notif = msg->cast<CFlowState::Notification>();
		if (notif->event == CFlowState::ev_flowControl) {
			if ((getFlowState() != NULL) &&
				(getFlowState()->getOperationalState() != CFlowState::s_stale) &&
				getFlowState()->canSendOutgoingPackets())
			{
				unique_lock<mutex> blockingVariables;

				if (appRecvBlocked) {
					appRecvBlocked = false;

	//				DBG_DEBUG(FMT("%1%: unblocked") % getId());

					if (!readFromAppInProgress && (header[0] != MSG_TYPE_END)) {
						readFromAppInProgress = true;
						start_header_receive();
					}
				}

			}

		} else if (notif->event == CFlowState::ev_stateChanged) {
			if (getFlowState() != NULL) {
				if (getFlowState()->getErrorState() == CFlowState::e_reset) {
					string emsg("communication reset by peer");
					sendPayload(MSG_TYPE_ERR, shared_buffer_t(emsg.c_str()));

				} else if (getFlowState()->getErrorState() == CFlowState::e_timeout) {
					string emsg("communication timed out");
					sendPayload(MSG_TYPE_ERR, shared_buffer_t(emsg.c_str()));

				} else if (getFlowState()->getErrorState()) {
					string emsg("communication error");
					sendPayload(MSG_TYPE_ERR, shared_buffer_t(emsg.c_str()));

				}

				releaseOnSendComplete = true;

			}

		}

	} else if (ev->getId() == EVENT_NETLETSELECTOR_APPCONNREADY) {
		assert(getFlowState().get() != NULL);
		string properties = (FMT("{ \"uri\"  : \"%1%\"}") % getFlowState()->getRequestUri()).str();
			//"{ \"test\": \"works\" }";
		sendPayload(MSG_TYPE_REQ, shared_buffer_t(properties.c_str()));
		DBG_DEBUG(boost::format("%1%: sent properties to application") % getId());

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
void CSocketAppConnector::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	IEnhancedAppConnector::processTimer(msg);
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * This is the most common case of a application induced message for this NetAdapt.
 *
 * @param msg	Pointer to message
 */
void CSocketAppConnector::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	assert(false);
}

/* ========================================================================= */

CAppSocketServer::CAppSocketServer(CNena * nodearch, IMessageScheduler * sched,
		boost::shared_ptr<boost::asio::io_service> ios) :
		IEnhancedAppServer(sched), nodearch(nodearch), io_service(ios), ac(*ios)
{
	className += "::CAppSocketServer";
	setId(serverName);

	string socketName = BOOSTSOCK_DEFAULT_SOCKET;

	if (nodearch->getConfig()->hasParameter(getId(), "socketName")) {
		string name;
		nodearch->getConfig()->getParameter(getId(), "socketName", name);

		if (!name.empty())
			socketName = name;
	}

	if (boost::filesystem::exists(socketName)) {
		try {
			boost::filesystem::remove(socketName);

		} catch (std::exception& e) {
			DBG_FAIL(FMT("%1%: cannot delete socket file '%2%': %3%") % getId() % socketName % e.what());

		}

	}

	stream_protocol::endpoint endp(socketName);
	boost::system::error_code er;
	ac.open(endp.protocol(), er);
	if (er) {
		DBG_WARNING(FMT("%1%: error in opening %2%: %3%") % getId() % socketName % er.message());
	}
	ac.bind(endp, er);
	if (er) {
		DBG_WARNING(FMT("%1%: error in binding %2%: %3%") % getId() % socketName % er.message());
	}
	ac.listen();

	boost::asio::socket_base::receive_buffer_size sopt_rbuf(BOOSTSOCK_SYSRECVBUFSIZE);
	boost::asio::socket_base::send_buffer_size sopt_sbuf(BOOSTSOCK_SYSSENDBUFSIZE);

//	ac.set_option(sopt_rbuf);
//	ac.set_option(sopt_sbuf);

	ac.get_option(sopt_rbuf);
	ac.get_option(sopt_sbuf);

	DBG_DEBUG(
			FMT("%1%: listening on %2% (rbuf %3%, sbuf %4%)...") % getId() % socketName % sopt_rbuf.value() % sopt_sbuf.value());
}

void CAppSocketServer::handle_accept(boost::shared_ptr<IEnhancedAppConnector> old_con,
		const boost::system::error_code& error)
{
	if (!error) {
//		DBG_DEBUG(boost::format("%1%: new application connection") % getId());

		dynamic_cast<CSocketAppConnector *>(old_con.get())->start();
		addNew(old_con);
		boost::shared_ptr<IEnhancedAppConnector> new_con(
				new CSocketAppConnector(nodearch, scheduler, io_service, this));
		ac.async_accept(dynamic_cast<CSocketAppConnector *>(new_con.get())->getSocket(),
				boost::bind(&CAppSocketServer::handle_accept, this, new_con, boost::asio::placeholders::error));
	} else {
		DBG_WARNING(FMT("%1%: error in accept: %2%") % getId() % error.message());
	}
}
;

void CAppSocketServer::start()
{

	boost::shared_ptr<IEnhancedAppConnector> new_con(new CSocketAppConnector(nodearch, scheduler, io_service, this));
	CSocketAppConnector * tmp = dynamic_cast<CSocketAppConnector *>(new_con.get());
	if (tmp == NULL)
		DBG_FAIL(FMT("%1%: dynamic_cast of CSocketAppConnector failed.") % getId());
	else {
		ac.async_accept(tmp->getSocket(),
				boost::bind(&CAppSocketServer::handle_accept, this, new_con, boost::asio::placeholders::error));

		DBG_DEBUG(boost::format("%1%: running...") % getId());
	}
}

void CAppSocketServer::stop()
{
	ac.cancel();
	DBG_DEBUG(boost::format("%1%: stopping I/O") % getId());
}

void CAppSocketServer::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CAppSocketServer::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CAppSocketServer::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CAppSocketServer::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}
