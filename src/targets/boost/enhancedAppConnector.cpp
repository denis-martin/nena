#include "enhancedAppConnector.h"
#include "nena.h"

#include "messages.h"
#include "messageBuffer.h"
#include "debug.h"
#include "msg.h"

#include <string>

#include <boost/enable_shared_from_this.hpp>

using namespace std;
using boost::shared_ptr;

IEnhancedAppConnector::IEnhancedAppConnector(IMessageScheduler* sched, CNena * na, IAppServer * server) :
				IAppConnector(sched),
				nodearch(na),
				appServer(server),
				connectorTest(false),
				extConnectorTest(false),
				appRecvBlocked(false),
				userRequestId(0)
{
	className += "::IEnhancedAppConnector";

	netletSelector = static_cast<CNetletSelector*>(nodearch->lookupInternalService(
			"internalservice://nena/netletSelector"));
	assert(netletSelector != NULL);
}

IEnhancedAppConnector::~IEnhancedAppConnector()
{
	if (getFlowState().get() != NULL) {
		DBG_DEBUG(boost::format("%1%: destroyed [%2%(%3%)]") % getId() % getRemoteURI() % getFlowState()->getFlowId());
		nodearch->releaseFlowState(flowState);
		flowState.reset();

	} else {
		DBG_DEBUG(boost::format("%1%: destroyed [%2%(-)]") % getId() % getRemoteURI());

	}
}

void IEnhancedAppConnector::registerMe()
{
	assert(!endOfLife);
	if (!registered) {
		netletSelector->registerAppConnector(this);
		getFlowState()->registerListener(this);

	}
}

void IEnhancedAppConnector::unregisterMe()
{
	if (registered) {
		getFlowState()->unregisterListener(this);
		netletSelector->unregisterAppConnector(this);
		registered = false;

	}
}

void IEnhancedAppConnector::handlePayload(MSG_TYPE type, shared_buffer_t payload)
{
//	DBG_DEBUG(FMT("%1%(%2%) received message type %3%") % className % identifier % type);

	switch (type) {
	case MSG_TYPE_DATA:
		if (registered) {
			shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(this, getNext()));
//			DBG_INFO(FMT("pkt->size(): %1%, pkt->get_cursor(): %2%") % pkt->size() % pkt->get_cursor());
			pkt->setType(IMessage::t_outgoing);
			pkt->push_back(payload);
//			DBG_INFO(FMT("pkt->size(): %1%, pkt->get_cursor(): %2%") % pkt->size() % pkt->get_cursor());

//			DBG_DEBUG(FMT("%1%(%2%) received data message") % className % identifier);

			if (extConnectorTest || connectorTest) {
				// for testing, redirect the packet to us
				pkt->setTo(this);

			} else {
				if (!getFlowState()->getRemoteId().empty()) {
					pkt->setProperty(IMessage::p_destId, new CStringValue(getFlowState()->getRemoteId()));

				} else {
					pkt->setProperty(IMessage::p_destId, new CStringValue(getRemoteURI()));

				}

				pkt->setProperty(IMessage::p_serviceId, new CStringValue(getRemoteURI()));
				pkt->setProperty(IMessage::p_srcId, new CStringValue(nodearch->getNodeName()));
				pkt->setProperty(IMessage::p_method, new CIntValue(getMethod()));
				pkt->setFlowState(getFlowState());
			}

			if (connectorTest && !extConnectorTest)
				processIncoming(pkt);
			else {
//					DBG_DEBUG(FMT("%1%(%2%) sending packet to next: %3%") % className % identifier % getNext()->getClassName());

				shared_ptr<CFlowState> fs = getFlowState();
				FLOWSTATE_FLOATOUT_INC(fs, 1, "app", nodearch->getSysTime());

				sendMessage(pkt);

				if (!fs->canSendOutgoingPackets()) {
//					DBG_INFO(FMT("%1%: exceeding maximum allowed number of outgoing floating packets (%2% / %3%)") %
//							getId() % fs->getOutFloatingPackets() % fs->getOutMaxFloatingPackets());

					appRecvBlocked = true;

				}

			}

		} else {
			DBG_WARNING(FMT("%1%: %2% received data message but is not registered") % getId() % identifier);
		}
		break;

	case MSG_TYPE_CONNECTORTEST:
		this->setConnectorTest(static_cast<bool>(payload[0]));
		DBG_DEBUG(boost::format("%1%: setting connectorTest to %2%") % getId() % this->getConnectorTest());
		break;
	case MSG_TYPE_EXTCONNECTORTEST:
		this->setExtConnectorTest(static_cast<bool>(payload[0]));
		DBG_DEBUG(boost::format("%1%: setting extConnectorTest to %2%") % getId() % this->getExtConnectorTest());
		break;

	case MSG_TYPE_TARGET:
		setRemoteURI(string(reinterpret_cast<const char*>(payload.data()), payload.size()));
		break;
	case MSG_TYPE_ID:
		setIdentifier(string(reinterpret_cast<const char*>(payload.data()), payload.size()));
		break;

	case MSG_TYPE_GET: {
		setMethod(method_get);
		setRemoteURI(string(reinterpret_cast<const char*>(payload.data()), payload.size()));
		DBG_DEBUG(boost::format("%1%: setting method to %2% (GET %3%).") % getId() % getMethod() % getRemoteURI());
		break;
	}
	case MSG_TYPE_PUT: {
		setMethod(method_put);
		setRemoteURI(string(reinterpret_cast<const char*>(payload.data()), payload.size()));
		DBG_DEBUG(boost::format("%1%: setting method to %2% (PUT %3%)") % getId() % getMethod() % getRemoteURI());
		break;
	}
	case MSG_TYPE_CONNECT: {
		setMethod(method_connect);
		setRemoteURI(string(reinterpret_cast<const char*>(payload.data()), payload.size()));
		DBG_DEBUG(boost::format("%1%: setting method to %2% (CONNECT %3%)") % getId() % getMethod() % getRemoteURI());
		break;
	}
	case MSG_TYPE_BIND: {
		setMethod(method_bind);
		setRemoteURI(string(reinterpret_cast<const char*>(payload.data()), payload.size()));
		DBG_DEBUG(boost::format("%1%: setting method to %2% (BIND %3%)") % getId() % getMethod() % getRemoteURI());
		break;
	}
	case MSG_TYPE_END: {
		DBG_DEBUG(boost::format("%1%: got end message.") % getId());

		if (getFlowState()->getOperationalState() == CFlowState::s_valid) {
			// send end-of-stream marker
//			DBG_DEBUG(boost::format("%1%: generating end-of-stream marker") % getId());

			shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(this, getNext()));
			pkt->setType(IMessage::t_outgoing);

			if (!getFlowState()->getRemoteId().empty())
				pkt->setProperty(IMessage::p_destId, new CStringValue(getFlowState()->getRemoteId()));
			else
				pkt->setProperty(IMessage::p_destId, new CStringValue(getRemoteURI()));

			pkt->setProperty(IMessage::p_serviceId, new CStringValue(getRemoteURI()));
			pkt->setProperty(IMessage::p_srcId, new CStringValue(nodearch->getNodeName()));
			pkt->setProperty(IMessage::p_method, new CIntValue(getMethod()));
			pkt->setProperty(IMessage::p_endOfStream, new CBoolValue(true));
			pkt->setFlowState(getFlowState());

			getFlowState()->incOutFloatingPackets();
			getFlowState()->setOperationalState(this, CFlowState::s_end);

			sendMessage(pkt);

		}

		release();
		break;
	}
	case MSG_TYPE_USERREQ: {
		// user request reply
		CMessageBuffer buf;
		buf.push_back(payload);
		uint32_t id = buf.pop_ulong();
		userreq_t answer = (userreq_t) buf.pop_ulong();

		map<uint32_t, UserRequest>::iterator it = userRequests.find(id);
		if (it == userRequests.end()) {
			DBG_WARNING(FMT("User request ID %1% not found.") % id);

		} else {
			shared_ptr<IMessage> ura(
					new Message_UserRequestAnswer(it->second.requestId, answer, this, it->second.replyTo));
			sendMessage(ura);
			userRequests.erase(it);

		}


		break;
	}
	case MSG_TYPE_REQ: {
		// requirements message
		DBG_DEBUG(boost::format("%1%: got requirements from application %2%") % getId() %
				string(reinterpret_cast<const char*>(payload.data()), payload.size()));

		// store requirements in app connector
		string reqs = string(reinterpret_cast<const char*>(payload.data()), payload.size());
		setRequirements(reqs);

		break;
	}
	default:
		DBG_WARNING(boost::format("%1%(%2%): unknown message type %3%!") % className % identifier % type);
		break;
	}

	// if not done yet, register
	if (!registered && !identifier.empty() && method != method_none && !endOfLife)
		registerMe();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void IEnhancedAppConnector::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<ReleaseTimer> releaseTimer = msg->cast<ReleaseTimer>();
	if (releaseTimer.get() == NULL) {
		DBG_ERROR("Unhandled timer!");
		throw EUnhandledMessage();

	}

	dynamic_cast<IEnhancedAppServer *>(appServer)->destroyAppConnector(shared_from_this());
}

void IEnhancedAppConnector::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	assert(false);
}

void IEnhancedAppConnector::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL)
		DBG_FAIL("Incoming message not a CMessageBuffer!");

	if ((getFlowState() != NULL) && (getFlowState()->getOperationalState() != CFlowState::s_stale)) {
		MSG_TYPE type = MSG_TYPE_DATA;

		shared_ptr<CMorphableValue> mv;
		if (pkt->hasProperty(IMessage::p_endOfStream, mv)) {
			if (mv->cast<CBoolValue>()->value()) {
				type = MSG_TYPE_END;
//				DBG_DEBUG(FMT("%1%(%2%): got incoming end-of-stream marker") % getId() % pkt->getFlowState()->getFlowId());
			}
		}

		shared_buffer_t payload = pkt->getBuffer().linearize();
		if (payload.size() > 0 || type == MSG_TYPE_END) {
			if (type == MSG_TYPE_END && payload.size() > 0) {
				DBG_DEBUG(FMT("%1% got end message with data:\n%2%") % getId() % pkt->to_str());
				assert(false);
			}

			sendPayload(type, payload);
//			DBG_DEBUG(FMT("%1%: sent data to application") % getId());

		} /* else {
			DBG_DEBUG(FMT("%1%: dropping empty packet") % getId());

		} */

		getFlowState()->decInFloatingPackets();

	} else {
		string m = str(FMT("%1% incoming msg: app connector has no valid flow state") % getId());
		throw EUnhandledMessage(m);

	}
}

/**
 * @brief	Present installation request to the user
 *
 * @param msgstr		String to present to the user
 * @param requestType	Sum/arithmetic OR of reply options, see IAppConnector::userreq_t
 * @param replyTo		Message processor to reply to
 * @param requestId		Optional request ID
 */
void IEnhancedAppConnector::sendUserRequest(const std::string& msgstr, userreq_t requestType,
		IMessageProcessor *replyTo, uint32_t requestId)
{
	uint32_t id = ++userRequestId;
	userRequests[id] = UserRequest(replyTo, requestId);

	MSG_TYPE type = MSG_TYPE_USERREQ;
	size_t size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + msgstr.size();

	CMessageBuffer payload(size);
	payload.push_ulong(id);
	payload.push_ulong(requestType);
	payload.push_ulong(msgstr.size());
	payload.push_string(msgstr);

	assert(payload.getBuffer().length() == 1);
	sendPayload(type, payload.getBuffer().at(0));
}

/**
 * @brief	Send USER_1 signal; temporarily added
 *
 * @param message 	User message
 */
void IEnhancedAppConnector::sendUser1(boost::shared_ptr<CMessageBuffer> message)
{
	sendPayload(MSG_TYPE_USER_1, message->getBuffer().linearize());
}

/**
 * @brief	Send an event to the application
 *
 * @param event		Event ID
 * @param param		Parameter for the event
 */
void IEnhancedAppConnector::sendEvent(event_t event, const std::string& param)
{
	switch (event) {
	case event_incoming: {
		sendPayload(MSG_TYPE_EVENT_INCOMING, shared_buffer_t(param.c_str()));
		break;
	}
	default: {
		DBG_WARNING(FMT("%1%: unknown event %2%") % getId() % event);
		break;
	}
	}
}

/**
 * @brief	Release / close app connector
 */
void IEnhancedAppConnector::release()
{
	if (!endOfLife) {
		endOfLife = true;
		unregisterMe();
		dynamic_cast<IEnhancedAppServer *>(appServer)->notifyEnd(shared_from_this());
		scheduler->setTimer(new ReleaseTimer(this));
	}
}
