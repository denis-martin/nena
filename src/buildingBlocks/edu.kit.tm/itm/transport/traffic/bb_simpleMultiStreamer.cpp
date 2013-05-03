/*
 * bb_simpleMultiStreamer.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: denis
 */

#include "bb_simpleMultiStreamer.h"

#include "nena.h"
#include "netletSelector.h"

namespace edu_kit_tm {
namespace itm {
namespace transport {

using namespace std;
using boost::shared_ptr;

const std::string simpleMultiStreamerClassName = "bb://edu.kit.tm/itm/transport/traffic/simpleMultiStreamer";

class SimpleMultiStreamHeader : public IHeader
{
public:
	Bb_SimpleMultiStreamer::MsgTypes msgType;

	SimpleMultiStreamHeader(
		Bb_SimpleMultiStreamer::MsgTypes msgType = Bb_SimpleMultiStreamer::msgType_Data
	) : msgType(msgType)
	{};

	virtual ~SimpleMultiStreamHeader() {};

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(sizeof(uint32_t) + sizeof(unsigned char)));
		buffer->push_ulong(0xdeadbeef);
		buffer->push_uchar((unsigned char) msgType);
		return buffer;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		assert(buffer->pop_ulong() == 0xdeadbeef);
		msgType = (Bb_SimpleMultiStreamer::MsgTypes) buffer->pop_uchar();
	}
};

class SimpleMultiStreamHeader_Register : public IHeader
{
public:
	std::string nodeName;
	std::string remoteId; // remote ID, either video:// or node://

	SimpleMultiStreamHeader_Register(
		std::string nodeName = std::string(),
		std::string remoteId = std::string()
	) : nodeName(nodeName), remoteId(remoteId)
	{};

	virtual ~SimpleMultiStreamHeader_Register() {};

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(2*sizeof(ushort) + nodeName.size() + remoteId.size()));
		buffer->push_ushort((ushort) nodeName.size());
		buffer->push_string(nodeName);
		buffer->push_ushort((ushort) remoteId.size());
		buffer->push_string(remoteId);
		return buffer;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		nodeName = buffer->pop_string((size_t) buffer->pop_ushort());
		remoteId = buffer->pop_string((size_t) buffer->pop_ushort());
	}
};

class SimpleMultiStreamHeader_Ack : public IHeader
{
public:
	std::string remoteId; // remote ID, either video:// or node://

	SimpleMultiStreamHeader_Ack(
		std::string remoteId = std::string()
	) : remoteId(remoteId)
	{};

	virtual ~SimpleMultiStreamHeader_Ack() {};

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{

		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(sizeof(ushort) + remoteId.size()));
		buffer->push_ushort((ushort) remoteId.size());
		buffer->push_string(remoteId);
		return buffer;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		remoteId = buffer->pop_string((size_t) buffer->pop_ushort());
	}
};

class SimpleMultiStreamHeader_Unregister : public IHeader
{
public:
	std::string nodeName;

	SimpleMultiStreamHeader_Unregister(
		std::string nodeName = std::string()
	) : nodeName(nodeName)
	{};

	virtual ~SimpleMultiStreamHeader_Unregister() {};

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(sizeof(ushort) + nodeName.size()));
		buffer->push_ushort((ushort) nodeName.size());
		buffer->push_string(nodeName);
		return buffer;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		nodeName = buffer->pop_string((size_t) buffer->pop_ushort());
	}
};

Bb_SimpleMultiStreamer::Bb_SimpleMultiStreamer(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const string id)
	: IBuildingBlock(nodeArch, sched, netlet, id)
{
	className += "::Bb_SimpleMultiStreamer";
	nodeArch->getNetletSelector()->registerListener(EVENT_NETLETSELECTOR_NETLETCHANGED, this);
	nodeArch->getNetletSelector()->registerListener(EVENT_NETLETSELECTOR_APPLICATIONDISCONNECT, this);
}

Bb_SimpleMultiStreamer::~Bb_SimpleMultiStreamer()
{
	clients.clear();
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleMultiStreamer::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> ev = msg->cast<IEvent>();
	if (ev->getId() == EVENT_NETLETSELECTOR_NETLETCHANGED) {
		shared_ptr<CNetletSelector::Event_NetletChanged> event_nch = msg->cast<CNetletSelector::Event_NetletChanged>();

		if (event_nch->appConn->getNetlet() == netlet) {
			// we have been selected
			string remoteId = event_nch->appConn->getRemoteURI();
			string remoteNode = remoteId;
			if (remoteId.compare(0, 8, "video://") == 0) {
				// replace video:// by node:// and chop off last part of URI
				string videoName;
				size_t right = remoteId.rfind('/');
				if (right < remoteId.size())
					videoName = remoteId.substr(right+1, remoteId.size()-right);

				if (right >= 8) {
					remoteNode = "node://" + remoteId.substr(8, right-8);
				} else {
					remoteNode = "node://";
				}

				DBG_DEBUG(FMT("%1%: Resolving %2% into (%3%, %4%)") % getClassName() %
						remoteId % remoteNode % videoName);
			}

			if (event_nch->appConn->getMethod() == IAppConnector::method_bind) {
				DBG_DEBUG(FMT("%1%: Published %2%") % getClassName() % remoteId);

			} else {
				DBG_DEBUG(FMT("%1%: Sending register request to %2%") % getClassName() % remoteNode);
				shared_ptr<CMessageBuffer> pkt = SimpleMultiStreamHeader_Register(nodeArch->getNodeName(), remoteId).serialize();
				pkt->push_header(SimpleMultiStreamHeader(msgType_Register));
				pkt->setFrom(this);
				pkt->setTo(next);
				pkt->setProperty(IMessage::p_destId, new CStringValue(remoteNode));

				sendMessage(pkt);

				shared_ptr<Timer> timer(new Timer(0.25, this, event_nch->appConn));
				scheduler->setTimer<Timer>(timer);

			}

			bool found = false;
			list<AppItem>::iterator it;
			for (it = registeredApps.begin(); it != registeredApps.end(); it++) {
				if (it->appIdentifier == event_nch->appConn->getIdentifier()) {
					found = true;
					*it = AppItem(remoteNode, remoteId, event_nch->appConn->getIdentifier(), event_nch->appConn);
					DBG_DEBUG(FMT("%1%: Updated app %2%") % getClassName() % event_nch->appConn->getIdentifier());
				}
			}

			if (!found)
				registeredApps.push_back(AppItem(remoteNode, remoteId, event_nch->appConn->getIdentifier(), event_nch->appConn));


		} else {
			list<AppItem>::iterator it;
			for (it = registeredApps.begin(); it != registeredApps.end(); it++) {
				if (it->appConn == event_nch->appConn) {
					// another netlet was selected, send unregister request
					shared_ptr<CMessageBuffer> pkt = SimpleMultiStreamHeader_Unregister(nodeArch->getNodeName()).serialize();
					pkt->push_header(SimpleMultiStreamHeader(msgType_Unregister));
					pkt->setFrom(this);
					pkt->setTo(next);
					pkt->setProperty(IMessage::p_destId, new CStringValue(it->remoteNode));
					sendMessage(pkt);

					registeredApps.erase(it);
					break;

				}

			}

		}

	} else if (ev->getId() == EVENT_NETLETSELECTOR_APPLICATIONDISCONNECT) {
		shared_ptr<CNetletSelector::Event_ApplicationDisconnect> event_disc =
				msg->cast<CNetletSelector::Event_ApplicationDisconnect>();

		if (event_disc->netletId == netlet->getMetaData()->getId()) {
			list<AppItem>::iterator it;
			for (it = registeredApps.begin(); it != registeredApps.end(); it++) {
				if (it->appIdentifier == event_disc->identifier) {
					DBG_DEBUG(FMT("%1%: Received ApplicationDisconnect event for %2%") %
							getClassName() %
							it->appIdentifier);

					if (it->method != IAppConnector::method_bind) {
						shared_ptr<CMessageBuffer> pkt = SimpleMultiStreamHeader_Unregister(nodeArch->getNodeName()).serialize();
						pkt->push_header(SimpleMultiStreamHeader(msgType_Unregister));
						pkt->setFrom(this);
						pkt->setTo(next);
						pkt->setProperty(IMessage::p_destId, new CStringValue(it->remoteNode));

						sendMessage(pkt);

						it->unregistering = true;
						it->retriesLeft = SIMPLEMULTISTREAMER_REGISTER_MAX_RETRIES;

						shared_ptr<UnregisterTimer> timer(new UnregisterTimer(0.25, this, it->appIdentifier));
						scheduler->setTimer<UnregisterTimer>(timer);

					} else {
						registeredApps.erase(it);

					}
					break;

				}

			}

		}

	} else {
		throw EUnhandledMessage("Received unknown event message");

	}
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleMultiStreamer::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<Timer> timer = msg->cast<Timer>();
	if (timer.get() != NULL) {
		list<AppItem>::iterator it;
		for (it = registeredApps.begin(); it != registeredApps.end(); it++) {
			if (it->appConn == timer->appConn) {
				DBG_DEBUG(FMT("Register timer: App %1%") % it->appIdentifier);
				if (!it->registerAcknowledged) {
					if (it->retriesLeft > 0) {
						// resending register request
						DBG_DEBUG(FMT("%1%: Resending register request for %2% (ack state %3%)") %
								getClassName() % it->remoteId % it->registerAcknowledged);
						shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(this, next));
						pkt->push_header(SimpleMultiStreamHeader_Register(nodeArch->getNodeName(), it->remoteId));
						pkt->push_header(SimpleMultiStreamHeader(msgType_Register));

						pkt->setProperty(IMessage::p_destId, new CStringValue(it->remoteNode));

						sendMessage(pkt);
						scheduler->setTimer<Timer>(timer);

						it->retriesLeft--;

					} else {
						DBG_DEBUG(FMT("%1%: Giving up register request to %2%") % getClassName() % it->remoteNode);
						it->appConn->sendUserRequest(
								(FMT("Destination unreachable:\n\n%1%") % it->remoteNode).str(),
								IAppConnector::userreq_cancel, this, 0);

						registeredApps.erase(it);

					}

				}

				break; // break from for

			}

		}

	} else {
		shared_ptr<UnregisterTimer> urtimer = msg->cast<UnregisterTimer>();
		if (urtimer.get() != NULL) {
			list<AppItem>::iterator it;
			for (it = registeredApps.begin(); it != registeredApps.end(); it++) {
				if (it->unregistering && it->appIdentifier == urtimer->appId) {
					DBG_DEBUG(FMT("Unregister timer: App %1%") % it->appIdentifier);
					if (it->retriesLeft > 0) {
						// resending unregister request
						DBG_DEBUG(FMT("%1%: Resending unregister request for %2%") %
								getClassName() % it->remoteId);
						shared_ptr<CMessageBuffer> pkt = SimpleMultiStreamHeader_Unregister(nodeArch->getNodeName()).serialize();
						pkt->push_header(SimpleMultiStreamHeader(msgType_Unregister));
						pkt->setFrom(this);
						pkt->setTo(next);
						pkt->setProperty(IMessage::p_destId, new CStringValue(it->remoteNode));

						sendMessage(pkt);
						scheduler->setTimer<UnregisterTimer>(urtimer);

						it->retriesLeft--;

					} else {
						DBG_DEBUG(FMT("%1%: Giving up unregister request to %2%") % getClassName() % it->remoteNode);
						registeredApps.erase(it);

					}

					break; // break from for

				}

			}

		} else {
			DBG_ERROR("Unhandled timer!");
			throw EUnhandledMessage();

		}

	}

}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleMultiStreamer::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);

	pkt->push_header(SimpleMultiStreamHeader(msgType_Data));
	pkt->setFrom(this);
	pkt->setTo(getNext());

//	DBG_DEBUG(FMT("Bb_SimpleMultiStreamer::processOutgoing()"));

	if (clients.size() > 0) {
		list<ClientItem>::const_iterator it;
		for (it = clients.begin(); it != clients.end(); it++) {
			list<ClientItem>::const_iterator tmpit = it;
			tmpit++;
			if (tmpit == clients.end()) {
				// last client, reuse packet
				pkt->setProperty<CStringValue>(IMessage::p_destId, new CStringValue(it->nodeName));
				sendMessage(pkt);

//				DBG_DEBUG(FMT("  Sent packet to last client"));

			} else {
				// duplicate packet
				shared_ptr<CMessageBuffer> tmppkt = pkt->clone();
				tmppkt->setProperty(IMessage::p_destId, new CStringValue(it->nodeName));
				sendMessage(tmppkt);

				DBG_DEBUG(FMT("  Duplicated packet"));

			}

		}

	} else {
//		sendMessage(pkt);
		DBG_DEBUG(FMT("  No clients..."));

	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleMultiStreamer::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	assert(pkt.get() != NULL);

	SimpleMultiStreamHeader hdr;
	pkt->pop_header(hdr);
	switch (hdr.msgType) {
		case (msgType_Data): {
			// hand to upper entity
			pkt->setFrom(this);
			pkt->setTo(getPrev());
			sendMessage(pkt);

			string appId = pkt->getProperty<CStringValue>(IMessage::p_serviceId)->value();
			list<AppItem>::iterator it;
			for (it = registeredApps.begin(); it != registeredApps.end(); it++) {
				if (it->appIdentifier == appId) {
					if (!it->registerAcknowledged) {
						it->registerAcknowledged = true;
						DBG_INFO(FMT("%1%: Register request implicitly acknowledged") % getClassName());
					}
					break;
				}
			}
			break;
		}
		case (msgType_Register): {
			SimpleMultiStreamHeader_Register hdrr;
			pkt->pop_header(hdrr);
			if (hdrr.nodeName.empty()) {
				DBG_ERROR(FMT("%1%: Received empty register message") % getClassName());

			} else {
				// check whether we have a publishing application
				IAppConnector* serverAppConn = NULL;
				list<AppItem>::iterator app_it;
				for (app_it = registeredApps.begin(); app_it != registeredApps.end(); app_it++) {
					if (app_it->appConn->getMethod() == IAppConnector::method_bind) {
						serverAppConn = app_it->appConn;
						break;
					}
				}

				bool found = false;
				std::list<ClientItem>::iterator it;
				for (it = clients.begin(); it != clients.end(); it++) {
					if (it->nodeName == hdrr.nodeName) {
						DBG_INFO(FMT("Node %1% already registered as receiver") % it->nodeName);
						found = true;

						if (it->requestedId != hdrr.remoteId) {
							// update requested ID
							DBG_INFO(FMT("Changing requested ID from %1% to %2%") % it->requestedId % hdrr.remoteId);
							it->requestedId = hdrr.remoteId;
							if (serverAppConn != NULL) {
								// update requested ID at application
								shared_ptr<CMessageBuffer> buf(new CMessageBuffer(hdrr.remoteId.size() /* + sizeof(uint32_t) */));
//								buf->push_ulong((uint32_t) hdrr.remoteId.size());
								buf->push_string(hdrr.remoteId);
								serverAppConn->sendUser1(buf);

							}

						}

						break;
					}
				}

				if (!found) {
					DBG_INFO(FMT("Node %1% registered as receiver") % hdrr.nodeName);
					clients.push_back(ClientItem(hdrr.nodeName, hdrr.remoteId));

					if (serverAppConn != NULL) {
						// update requested ID at application
						shared_ptr<CMessageBuffer> buf(new CMessageBuffer(hdrr.remoteId.size()));
						buf->push_string(hdrr.remoteId);
						serverAppConn->sendUser1(buf);

					}
				}

				shared_ptr<CMessageBuffer> ack = SimpleMultiStreamHeader_Ack(hdrr.remoteId).serialize();
				ack->push_header(SimpleMultiStreamHeader(msgType_Ack));
				ack->setFrom(this);
				ack->setTo(getNext());
				ack->setProperty(IMessage::p_destId, new CStringValue(hdrr.nodeName));
				sendMessage(ack);
			}

			break;
		}
		case (msgType_Unregister): {
			SimpleMultiStreamHeader_Unregister hdrr;
			pkt->pop_header(hdrr);
			if (hdrr.nodeName.empty()) {
				DBG_ERROR(FMT("%1%: Received empty unregister message") % getClassName());

			} else {
				bool found = false;
				std::list<ClientItem>::iterator it;
				for (it = clients.begin(); it != clients.end(); it++) {
					if (it->nodeName == hdrr.nodeName) {
						DBG_INFO(FMT("Unregistering node %1% as receiver") % it->nodeName);

						// send ACK
						shared_ptr<CMessageBuffer> ack = SimpleMultiStreamHeader_Ack(nodeArch->getNodeName()).serialize();
						ack->push_header(SimpleMultiStreamHeader(msgType_Ack));
						ack->setFrom(this);
						ack->setTo(getNext());
						ack->setProperty(IMessage::p_destId, new CStringValue(hdrr.nodeName));
						sendMessage(ack);

						if (clients.size() == 1) {
							// check whether we have a publishing application
							IAppConnector* serverAppConn = NULL;
							list<AppItem>::iterator app_it;
							for (app_it = registeredApps.begin(); app_it != registeredApps.end(); app_it++) {
								if (app_it->appConn->getMethod() == IAppConnector::method_bind) {
									serverAppConn = app_it->appConn;
									break;
								}
							}
							if (serverAppConn) {
								// stop playback
								shared_ptr<CMessageBuffer> buf(new CMessageBuffer(8));
								buf->push_string("video://");
								serverAppConn->sendUser1(buf);
							}
						}

						clients.erase(it);
						found = true;
						break;
					}
				}
				if (!found) {
					DBG_INFO(FMT("Unregister failed: node %1% not registered as receiver") % hdrr.nodeName);

					// send ACK anyway
					shared_ptr<CMessageBuffer> ack = SimpleMultiStreamHeader_Ack(nodeArch->getNodeName()).serialize();
					ack->push_header(SimpleMultiStreamHeader(msgType_Ack));
					ack->setFrom(this);
					ack->setTo(getNext());
					ack->setProperty(IMessage::p_destId, new CStringValue(hdrr.nodeName));
					sendMessage(ack);
				}
			}

			break;
		}
		case (msgType_Ack): {
			SimpleMultiStreamHeader_Ack hdr;
			pkt->pop_header(hdr);
			list<AppItem>::iterator it;
			for (it = registeredApps.begin(); it != registeredApps.end(); it++) {
				if (!it->unregistering && it->remoteId == hdr.remoteId) { // TODO: criteria not sufficient...
					DBG_INFO(FMT("Register request for %1% acknowledged (ack state %2%).") %
							hdr.remoteId % it->registerAcknowledged);
					it->registerAcknowledged = true;
					break;

				} else if (it->unregistering && it->remoteNode == hdr.remoteId) { // it->remoteNode is intentionalDBG_INFO(FMT("Register request for %1% acknowledged (ack state %2%).") %
					DBG_INFO(FMT("Unregister request for %1% acknowledged.") % hdr.remoteId);
					registeredApps.erase(it);
					break;

				}
			}
			break;
		}
		default: {
			throw EUnhandledMessage("Invalid message type");
		}
	}

}

}
}
}
