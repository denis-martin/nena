/*
* bb_hiimapUser.cpp
*
*  Created on: Jan 21, 2010
*      Author: hans
*/

#include "bb_hiimapUser.h"

#include "netAdaptBroker.h"

#include "archs/edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

#include "md5/md5.h"

namespace edu_kit_tm {
namespace itm {

using namespace std;
using namespace simpleArch;
using namespace pugi;
using boost::shared_ptr;

namespace sig {

const std::string hiimapUserClassName = "bb://edu.kit.tm/itm/sig/hiimapUser";

class HiiMap_RequestLocator: public IHeader
{
public:

	unsigned char version;
	unsigned char type;
	hiimap_uid_t uid;

	HiiMap_RequestLocator(
		shared_buffer_t uid = shared_buffer_t()
	) : version(0x01), type(0x31), uid(uid)
	{};

	virtual ~HiiMap_RequestLocator() {};

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		uint32_t payloadlength = uid.size()+16+4+4; // locator 16 extra bytes to "stretch" uid to 256bit, +4 for "no options" field // +4 belongs to the hack mentioned below TODO: check/remove?

		size_t s = 6 + payloadlength + 1;
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(s));

		buffer->push_uchar(version);
		buffer->push_uchar(type);
		buffer->push_ulong(payloadlength);

		buffer->push_buffer(uid.data(), uid.size());
		buffer->push_ulong(0); // "stretch" uid to 256bit, TODO: proper implementation
		buffer->push_ulong(0);
		buffer->push_ulong(0);
		buffer->push_ulong(0);
		buffer->push_ulong(0); // HACK? Empty locator string? TODO: Check with HiiMap spec and change if necessary
		buffer->push_ulong(0); // 0 for "no options"

		uint32_t payloadsum = 0;
		for (uint32_t i=0; i < uid.size(); i++) {
			payloadsum += uid[i];
		}
		uint32_t checksum = version + type + payloadlength + payloadsum;
		buffer->push_uchar((unsigned char) checksum);

		return buffer;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> mbuf)
	{
		// NOT NEEDED at the moment...
		assert(false);
	}
};

class HiiMap_LocatorResponse: public IHeader
{
public:

	unsigned char version;
	unsigned char type;
	hiimap_uid_t uid;
	std::string locator;
	std::list<std::string> options;

	HiiMap_LocatorResponse(
		shared_buffer_t uid = shared_buffer_t(16), //changed from 32 -> 16
		std::string locator = std::string(),
		std::list<std::string> options = std::list<std::string>()
	) : version(0x01), type(0x32), uid(uid), locator(locator), options(options)
	{};

	virtual ~HiiMap_LocatorResponse() {};

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		// Not needed at the moment
		assert(false);
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		DBG_DEBUG(FMT("Deserializing locator response."));

		//version = buffer->pop_uchar(); //already performed...
		//type = buffer->pop_uchar(); //...by handle_receive
		uint32_t payloadlength = buffer->pop_ulong();

		//buffer->pop_buffer(uid.mutable_data(), 32); //32 byte UID
		buffer->pop_buffer(uid.mutable_data(), 16); //16 byte of UID
		buffer->pop_ulong(); // ignore...
		buffer->pop_ulong(); // next...
		buffer->pop_ulong(); // 16...
		buffer->pop_ulong(); // bytes.
		uint32_t locatorlen = buffer->pop_ulong();
		DBG_DEBUG(FMT("Received locator. Length: %1%") % locatorlen);
		if (locatorlen > 0) {
			locator = buffer->pop_string(locatorlen);
		}

		// ignore next byte: undocumented byte used for load balancing within HiiMap
		// TODO: maybe this has to be changed again in the future!
		buffer->pop_uchar();

		// get number of options
		uint32_t numopts = buffer->pop_ulong();
		DBG_DEBUG(FMT("Received locator response. Number of options: %1%") % numopts);

		// get each option and append it to options list
		for(int i=0; i<numopts; i++) {
			// get length of current option
			uint32_t optlen = buffer->pop_ulong();
			// get option and append it to options list
			std::string opt = buffer->pop_string(optlen);
			DBG_DEBUG(FMT("Received locator response. Option: %1%") % opt);
			options.push_back(opt);
		}

		// ignore checksum. TODO: consider checksum?
	}
};

class HiiMap_UpdateLocator: public IHeader
{
public:

	unsigned char version;
	unsigned char type;
	hiimap_uid_t uid;
	std::string locator;

	HiiMap_UpdateLocator(
		shared_buffer_t uid = shared_buffer_t(),
		std::string locator = std::string()
	) : version(0x01), type(0x31), uid(uid), locator(locator)
	{};

	virtual ~HiiMap_UpdateLocator() {};

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		size_t s = 6 + 32 + 4 + locator.size() + 4 + 1;
		shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(s));

		// TODO: allow sending of options

		// standard version and type fields
		buffer->push_uchar(version);
		buffer->push_uchar(type);

		// payload length field
		uint32_t payloadlength = uid.size()+16; //identifier size + 16 extra bytes to "stretch" uid to 256bit
		payloadlength += locator.size()+4; // locator size + 4 byte for locator length field
		payloadlength += 4; // 4 byte for no options
		buffer->push_ulong(payloadlength);

		// uid
		buffer->push_buffer(uid.data(), 16);
		buffer->push_ulong(0); // "stretch" uid to 256bit, TODO: proper implementation
		buffer->push_ulong(0);
		buffer->push_ulong(0);
		buffer->push_ulong(0);

		// locator
		uint32_t locatorlen = locator.size();
		buffer->push_ulong(locatorlen);
		buffer->push_string(locator);

		// no options
		buffer->push_ulong(0); // 0 for "no options"

		// checksum
		uint32_t uidsum = 0;
		for (uint32_t i=0; i < uid.size(); i++) {
			uidsum += uid[i];
		}
		uint32_t locatorsum = 0;
		for (uint32_t i=0; i < locator.size(); i++) {
			locatorsum += locator.at(i);
		}

		uint32_t checksum = version + type + payloadlength + uidsum + locatorlen + locatorsum;
		buffer->push_uchar((unsigned char) checksum);

		return buffer;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> buffer)
	{
		// NOT NEEDED at the moment...
		assert(false);
	}
};


using boost::format;
using boost::asio::ip::udp;

Bb_HiimapUser::PollTimer::PollTimer(double timeout, IMessageProcessor *proc) : CTimer(timeout, proc) {}
Bb_HiimapUser::PollTimer::~PollTimer() {}

Bb_HiimapUser::Bb_HiimapUser(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id) :
IBuildingBlock(nodeArch, sched, netlet, id),
socket(io_service)
{
    className += "::Bb_HiimapUser";
    hiimapPort = 0;
    hiimapConnected = false;
    multiplexer = NULL;

    deployment.step = 0; // no running deployment process TODO: change in the future?

    shared_ptr<xml_document> config = netlet->getNetletConfig();

    string enable = config->child("netlet").child("specific").find_child_by_attribute("option", "name", "enable").child_value ();
    if (enable != "true" && enable != "1")
	{
    	DBG_DEBUG(FMT("%1% disabled") % className);
    	return;
    }

    repository = (INetletRepository*) nodeArch->lookupInternalService("internalservice://nena/repository");
    if (repository == NULL) {
    	DBG_WARNING(FMT("%1%: Error looking up repository, BB disabled.") % className);
    	return;
    }
    repository->registerListener(EVENT_REPOSITORY_MULTIPLEXERADDED, this);

    netletSelector = (CNetletSelector*) nodeArch->lookupInternalService("internalservice://nena/netletSelector");
	if (netletSelector == NULL) {
		DBG_WARNING(FMT("%1%: Error looking up Netlet selector, BB disabled.") % className);
		return;
	}
	netletSelector->registerListener(EVENT_NETLETSELECTOR_NOSUITABLENETLET, this);

    int port = 0;
    int localPort = 0;

    string hiimapHost = config->child("netlet").child("specific").find_child_by_attribute("option", "name", "host").child_value();
    if (hiimapHost.empty()) {
    	hiimapHost = "127.0.0.1";
    	DBG_INFO(FMT("HiiMap host not specified, using %1%") % hiimapHost);
    }

    string strPort = config->child("netlet").child("specific").find_child_by_attribute("option", "name", "port").child_value();
    if(!strPort.empty())
        port = atoi(strPort.c_str());
    if(port <= 0 || port >= (1 << 16)) {
        port = HIIMAP_PORT;
        DBG_INFO(FMT("HiiMap port not specified, using default HiiMap port %1%") % port);
    }
    
    strPort = config->child("netlet").child("specific").find_child_by_attribute("option", "name", "localPort").child_value();
    if(!strPort.empty())
        localPort = atoi(strPort.c_str());
    if(localPort <= 0 || localPort >= (1 << 16)) {
        localPort = HIIMAP_LOCALPORT;
        DBG_INFO(FMT("Local port not specified, using default HiiMap local port %1%") % localPort);
    }

//    boost::asio::io_service io_service;
//    udp::resolver resolver(io_service);
//    udp::resolver::query query(udp::v4(), hiimapHost);
//    hiimapEndpoint = *resolver.resolve(query);
//    hiimapEndpoint.port(port);

    hiimapEndpoint = udp::endpoint(
    	boost::asio::ip::address::from_string(hiimapHost), port);

    socket.open(udp::v4());
    socket.bind(udp::endpoint(udp::v4(), localPort));
    shared_ptr<CTimer> pollTimer(new PollTimer(HIIMAP_POLLTIMER, this));
    scheduler->setTimer(pollTimer);
    start_receive(); //receive packets
}

Bb_HiimapUser::~Bb_HiimapUser()
{
    nodeArch->getNetletSelector()->unregisterListener(EVENT_NETLETSELECTOR_NETLETCHANGED, this);
    io_service.stop();
}

void Bb_HiimapUser::start_receive()
{
    socket.async_receive_from(boost::asio::buffer(recv_buffer, BOOSTUDP_RECVBUFSIZE), sender,
                              boost::bind(&Bb_HiimapUser::handle_receive, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
}



void Bb_HiimapUser::sendDeployErrorToApp(std::string m)
{
	deployment.appCon->sendUserRequest(m,
		IAppConnector::userreq_cancel,
		this /* reply to */,
		0 /* optional request ID */);

}

void Bb_HiimapUser::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    boost::system::error_code err = error;
    shared_ptr<CMessageBuffer> buf;

    // process all waiting packets
    while ((bytes_transferred > 0) && !err) {
    	if (bytes_transferred < 4) {
    		DBG_WARNING(FMT("%1% received message too small (%2% bytes)") % getClassName() % bytes_transferred);

    	} else {
			// content of recv_buffer is copied
			buf = shared_ptr<CMessageBuffer>(new CMessageBuffer(buffer_t(bytes_transferred, reinterpret_cast<boctet_t*>(recv_buffer))));

			// TODO: improve packet handling
			//DBG_DEBUG(FMT("Received packet from remote host"));

			unsigned char version = buf->pop_uchar();

			if (version != 0x01) {
				DBG_DEBUG(FMT("Received packet with wrong version"));
				break;
			}

			unsigned char type = buf->pop_uchar();

			switch(type) {
			case 0x32: { // locator response

				DBG_DEBUG(FMT("Received locator response"));

				HiiMap_LocatorResponse locrespmsg;
				locrespmsg.deserialize(buf);

				/**********************************
				 *  NEW CODE FOR THE DEPLOYMENT
				 **********************************/
				// TODO: maybe this can all be merged with normal message handling and mapping entries

				// check if deployment process is running
				if (deployment.step > 0) {
					// deployment process is running

					if (deployment.step == 1) {
						// we are waiting for the resolution of the network entry

						// check if incoming reply is our network entry
						if (locrespmsg.uid == deployment.uid) {
							DBG_DEBUG(FMT("DEPLOYMENT: Received network entry"));
							// extract multiplexer and netlet names
							if (locrespmsg.options.size() == 0) {
								deployment.step = 0;
								sendDeployErrorToApp("Deployment Error - No Protocol found!");
								start_receive();
								return;
							}
							std::list<std::string>::iterator option;
							for (option = locrespmsg.options.begin(); option != locrespmsg.options.end(); option++) {
								//std::string option = locrespmsg.options.front();

								HiiMapDeployEntry entry = HiiMapDeployEntry();
								entry.name = *option;
								entry.locator = "";
								//entry.uid = shared_buffer_t(md5(option).data(), 16); //TODO: check! uid should be padded with zeros to reach 256 bit?
								//MD5 md5(*option);
								//entry.uid = shared_buffer_t((const char*) md5.digestBuf(), 16);
								entry.resolved = false;

								DBG_DEBUG(FMT("DEPLOYMENT: Network Entry - Option: %1%") % *option);
								if (option->compare(0, 7, "netlet=") == 0) {
									DBG_DEBUG(FMT("DEPLOYMENT: Network Entry - Option %1% is a Netlet") % *option);
									// netlet entry
									entry.name.erase(0, 7);
									MD5 md5(entry.name);
									entry.uid = shared_buffer_t((const char*) md5.digestBuf(), 16);
									deployment.netlets.push_back(entry);
									//deployment.netlets.sort(); // not possible...
									//deployment.netlets.unique(); //... with this atm.

									// send locator request
									sendLocatorRequest(entry.uid);
									DBG_DEBUG(FMT("DEPLOYMENT: Sent locator request for netlet"));
								} else if (option->compare(0, 12, "multiplexer=") == 0) {
									DBG_DEBUG(FMT("DEPLOYMENT: Network Entry - Option %1% is a Multiplexer") % *option);
									// multiplexer entry
									entry.name.erase(0,12);
									MD5 md5(entry.name);
									entry.uid = shared_buffer_t((const char*) md5.digestBuf(), 16);
									deployment.multiplexer = entry;

									// send locator request
									sendLocatorRequest(entry.uid);
									DBG_DEBUG(FMT("DEPLOYMENT: Sent locator request for multiplexer"));
								}

								//locrespmsg.options.pop_front();
							}

							// multiplexer and netlet entries created, proceed to step 2
							deployment.step = 2;
							DBG_DEBUG(FMT("DEPLOYMENT: Finished step 1."));

							// proceed to next packet
							start_receive();
							return;
						}
					} else if (deployment.step == 2) {
						// we are waiting for the resolution of the multiplexer and netlet entries

						bool found = false;
						// check if its the multiplexer
						if (locrespmsg.uid == deployment.multiplexer.uid) {
							found = true;

							deployment.multiplexer.locator = locrespmsg.locator;
							deployment.multiplexer.resolved = true;
							DBG_DEBUG(FMT("DEPLOYMENT: Step2 - Received Multiplexer Entry. Locator: %1%") % locrespmsg.locator);
						} else {
							// check if its a netlet
							std::list<HiiMapDeployEntry>::iterator netlet;
							for (netlet=deployment.netlets.begin(); netlet != deployment.netlets.end(); netlet++) {
								if (locrespmsg.uid == netlet->uid) {
									found = true;

									netlet->locator = locrespmsg.locator;
									netlet->resolved = true;
									DBG_DEBUG(FMT("DEPLOYMENT: Step2 - Received Netlet Entry. Locator: %1%") % locrespmsg.locator);
								}
							}
						}

						// check if everything is resolved now
						bool netletsResolved = true;
						std::list<HiiMapDeployEntry>::const_iterator netlet;
						for (netlet=deployment.netlets.begin(); netlet != deployment.netlets.end(); netlet++) {
							if (netlet->resolved == false) {
								netletsResolved = false;
							}
						}

						if (deployment.multiplexer.resolved == true && netletsResolved == true) {
							// everything is looked up
							// download multiplexer and netlets
							// load multiplexer and netlets
							// finish deployment process

							downloadComponents(); // download and load components

							//deployment.step = 0;
							DBG_DEBUG(FMT("DEPLOYMENT: Finished step 2."));
						}

						// if message belongs to deployment process, proceed to next message
						if (found == true) {
							start_receive();
							return;
						}

					} else {
						// undefined step
					}

				}

				/***************************
				 * Normal message handling
				 **************************/

//				DBG_DEBUG(FMT("Received locator response for UID %1%: %2%") % locrespmsg->uid.toStr() % locrespmsg->locator);
				//DBG_DEBUG(FMT("Received locator response for UID %1%") % locrespmsg->uid);
				//DBG_DEBUG(FMT("Received locator response"));
				//DBG_DEBUG(FMT("%1%") % locrespmsg->uid.toStr());
				HiiMapEntry& entry = mappings[locrespmsg.uid];
				if (entry.issuers.empty()) {
					// no issuers! error?
					DBG_DEBUG(FMT("Received locator response, but there's no issuer."));
					//DBG_DEBUG(FMT("%1%") % entry.name);
				} else {
					// there's someone interested in this response message
					// update mapping entry
					//entry.locators = locrespmsg->locator;
					shared_ptr<ipv4::CLocatorValue> loc(new ipv4::CLocatorValue(locrespmsg.locator));
					if (loc->isValid()) {
						entry.locators.list().push_back(loc);
						entry.locators.list().sort(); // sort and...
						entry.locators.list().unique(); // ...remove consecutive duplicates TODO: better solution?
						entry.timestamp = nodeArch->getSysTime();

						//ipv4::CLocatorList loclist;
						//loclist.list().push_back(ipv4::CLocatorValue(entry.locators));

//						DBG_DEBUG(FMT("Number of locators: %1%") % entry.locators.list().size());
//						DBG_DEBUG(FMT("Number of issuers: %1%") % entry.issuers.size());
//						DBG_DEBUG(FMT("Number of mapping entries: %1%") % mappings.size());

						//entry.issuers.sort(); // sort list and...
						//entry.issuers.unique(); //...remove consecutive duplicates TODO: better solution?
						while (entry.issuers.empty() == false) { // send locator to every issuer
							IMessageProcessor* issuer = entry.issuers.front(); // get issuer

							// construct reply message and send it to issuer
							shared_ptr<CSimpleNameAddrMapper::Event_ResolveLocatorResponse> resp(
												new CSimpleNameAddrMapper::Event_ResolveLocatorResponse(
														entry.name, entry.uid, entry.locators, this, issuer));
							sendMessage(resp);

							//DBG_DEBUG(FMT("Forwarded locator response to an issuer."));

							entry.issuers.pop_front(); //remove issuer from list
						}
					} else {
						DBG_DEBUG(FMT("WARNING: invalid locator received for UID %1%: %2%")
							% hiimap_uid_to_str(locrespmsg.uid) % locrespmsg.locator);
					}
				}

				break;
			}
			case 0x00: { // error message

				DBG_DEBUG(FMT("Received error message"));
				// TODO: Do something?

				break;
			}
			default: {
				DBG_DEBUG(FMT("Received unhandled or invalid hiimap packet"));
			}
			}
    	}

    	// grab next packet if available
    	if (socket.available() > 0) {
    		bytes_transferred = socket.receive_from(
    			boost::asio::buffer(recv_buffer, BOOSTUDP_RECVBUFSIZE), sender, 0, err);

    	} else {
    		break; // while

    	}

    }


    if (err) {
        DBG_WARNING(FMT("Bb_HiiMapUser: handle_receive failed with error code %1%") % err);
    }

    start_receive(); //receive next packet
}

/**
 * @brief Process a message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_HiimapUser::processMessage(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	if (msg->getType() == IMessage::t_message) {
		shared_ptr<IAppConnector::Message_UserRequestAnswer> ura = msg->cast<IAppConnector::Message_UserRequestAnswer>();
		assert(ura.get());

		DBG_DEBUG(FMT("Received UserRequestAnswer(%1%, %2%)") % ura->requestId % ura->answer);

		switch (ura->answer) {
		case IAppConnector::userreq_ok: {
			break;
		}
		case IAppConnector::userreq_cancel: {
			break;
		}
		case IAppConnector::userreq_yes: {
			// start deployment process TODO: remove hard-coded parts and make it more flexible

			DBG_DEBUG(FMT("User requested connection to a new network. Starting deployment process."));

			std::string networkName = "VideoNetwork";

			// create deployment entry
			deployment.step = 1; // set deployment process to "started"
			deployment.name = networkName;
			deployment.appCon = (IAppConnector*) ura->getFrom();
			//hiimap_uid_t uid = shared_buffer_t(md5(networkName).data(), 16); //TODO: check! uid should be padded with zeros to reach 256 bit?
			MD5 md5(networkName);
			hiimap_uid_t uid = shared_buffer_t((const char*) md5.digestBuf(), 16);
			deployment.uid = uid;
			deployment.multiplexer = HiiMapDeployEntry();
			deployment.multiplexer.name = "";
			deployment.multiplexer.uid = shared_buffer_t();
			deployment.multiplexer.locator = "";
			deployment.netlets = std::list<HiiMapDeployEntry>();

			// resolve network id in HiiMap
			sendLocatorRequest(deployment.uid);


			break;
		}
		case IAppConnector::userreq_no: {
			break;
		}
		default: {
			DBG_DEBUG(FMT("Unknown UserRequestAnswer %1% (id %2%)") % ura->answer % ura->requestId);
		}
		}

	} else {
		IBuildingBlock::processMessage(msg);

	}
}

void Bb_HiimapUser::sendLocatorRequest(hiimap_uid_t uid)
{
	// create new locator resolve message
	HiiMap_RequestLocator resolvemsg;
	resolvemsg.uid = uid;

	shared_ptr<CMessageBuffer> buf = resolvemsg.serialize();

	// send it
	boost::system::error_code err;
	socket.send_to(boost::asio::buffer(buf->getBuffer().at(0).data(), buf->getBuffer().size()), hiimapEndpoint, 0, err);

	DBG_DEBUG(FMT("Locator request sent to (remote) mapping service, UID size: %1%") % uid.size());
}

void Bb_HiimapUser::downloadComponents()
{
	// download the netlets and multiplexer from the repository
	// WGET SYNTAX: wget $REMOTENETLET -q -O $LOCALNETLET
	//				REMOTENETLET=server:port/path/and/filename
	//				LOCALNETLET=path/and/filename



	// download multiplexer TODO: FIXME!
	bool gotMultiplexer = false;
	std::string localRepository = "../../../build/archs/";
	std::string muxString = "wget ";
	int retval = -1;
	muxString.append(deployment.multiplexer.locator + ":8080/" + deployment.multiplexer.name);
	muxString.append(" -q -O ");
	muxString.append(localRepository + deployment.multiplexer.name);
	retval = system(muxString.data());
	if (retval == 0) {
		DBG_DEBUG(FMT("DEPLOYMENT: Downloaded Multiplexer %1% successfully.") % deployment.multiplexer.name);
		gotMultiplexer = true;
	} else {
		DBG_DEBUG(FMT("DEPLOYMENT: Problem downloading Multiplexer %1%.") % deployment.multiplexer.name);
	}

	// download netlets TODO: FIXME!
	bool gotNetlets = true;
	localRepository = "../../../build/netlets/";
	std::list<HiiMapDeployEntry>::const_iterator netlet;
	for (netlet=deployment.netlets.begin(); netlet != deployment.netlets.end(); netlet++) {
		std::string netletString = "wget ";
		netletString.append(netlet->locator + ":8080/" + netlet->name);
		netletString.append(" -q -O ");
		netletString.append(localRepository + netlet->name);
		retval = system(netletString.data());
		if (retval == 0) {
				DBG_DEBUG(FMT("DEPLOYMENT: Downloaded Netlet %1% successfully.") % netlet->name);
		} else {
				DBG_DEBUG(FMT("DEPLOYMENT: Problem downloading Netlet %1%.") % netlet->name);
				gotNetlets = false;
		}
	}

	// if we downloaded everything successfully...
	if (gotMultiplexer == true && gotNetlets == true) {
		// update local repository to actually load the multiplexer and netlets
		DBG_DEBUG(FMT("DEPLOYMENT: All components downloaded. Loading components"));
		repository->update();
		DBG_DEBUG(FMT("DEPLOYMENT: Loading components - update executed"));
		//DBG_DEBUG(FMT("DEPLOYMENT: Loading components - 1st update"));
		//repository->update();
		//DBG_DEBUG(FMT("DEPLOYMENT: Loading components - 2nd update"));

		// re-trigger netlet selection
		netletSelector->selectNetlet(deployment.appCon);
		DBG_DEBUG(FMT("DEPLOYMENT: Re-triggered netlet selection"));
	} else {
		DBG_DEBUG(FMT("DEPLOYMENT: Problem downloading components - New components not loaded."));
		sendDeployErrorToApp("Deployment Error - Error downloading Netlets!");
	}

	deployment.step = 0;
}


/**
* @brief Process an event message directed to this message processing unit
*
* @param msg	Pointer to message
*/
void Bb_HiimapUser::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
//	CSimpleNameAddrMapper::Event_ResolveLocatorRequest* ev =
//		dynamic_cast<CSimpleNameAddrMapper::Event_ResolveLocatorRequest*>(msg);

	shared_ptr<IEvent> event = msg->cast<IEvent>();

    if(event->getId() == EVENT_SIMPLEARCH_RESOLVELOCATORREQ) {
    	shared_ptr<CSimpleNameAddrMapper::Event_ResolveLocatorRequest> ev =
    			msg->cast<CSimpleNameAddrMapper::Event_ResolveLocatorRequest>();

        //DBG_DEBUG(FMT("%1%: Received %2% for ID %3%") % getClassName() % ev->name % ev->uid.toStr(false));

//
//        boost::system::error_code err;
//        std::string netletId = evChange->getProperty<CStringValue>(IMessage::p_netletId).value();
//
//        CSerialBuffer* buf = new CSerialBuffer();
//        buf->push_ulong(4);
//        buf->push_string(netletId);
//
//        socket.send_to(boost::asio::buffer(buf->getBuffer(), buf->size()), hiimapEndpoint, 0, err);
        
        //ipv4::CLocatorList loclist;
        //loclist.list().push_back(ipv4::CLocatorValue("127.0.0.1:1234"));

        //CSimpleNameAddrMapper::Event_ResolveLocatorResponse* resp =
        //	new CSimpleNameAddrMapper::Event_ResolveLocatorResponse(
        //		ev->name, ev->uid, loclist, this, ev->getFrom());
        //sendMessage(resp);

        // mapping system
        hiimap_uid_t uid = ev->uid;

        // not allowed for shared_buffer_t
        //uid.resize(32, 0); // fill with zeros TODO: better solution

        //DBG_DEBUG(FMT("Request for UID %1%") % uid.toStr());
        HiiMapEntry& entry = mappings[uid];
        if (entry.name.empty()) {
        //if ( entry.issuers.empty() ) { // initialize entry if it didn't exist TODO: better solution
        	entry.name = ev->name; // Not good. Multiple Names using same UID not considered
        	entry.uid = ev->uid;
        	entry.timestamp = 0;
        	entry.lastRequest = nodeArch->getSysTime();
        	entry.issuers.push_back(ev->getFrom());
        	entry.issuers.sort(); //this should not...
        	entry.issuers.unique(); //...be necessary

        	//DBG_DEBUG(FMT("%1%") % entry.name);

        	sendLocatorRequest(ev->uid);

        } else { // entry already exists

        	if (entry.locators.list().empty() || (nodeArch->getSysTime() - entry.timestamp > HIIMAP_ENTRYTIMEOUT)) { // no locators or locators too old
        		entry.issuers.push_back(ev->getFrom());
        		entry.issuers.sort(); // sort and..
        		entry.issuers.unique(); // ... remove duplicates TODO: better solution?

        		// re-issue request to remote mapping service
        		if (nodeArch->getSysTime() - entry.lastRequest > HIIMAP_REQTIMEOUT) {

        			entry.lastRequest = nodeArch->getSysTime();

        			// create new locator resolve message
					//HiiMap_RequestLocator resolvemsg;
					//resolvemsg.uid = ev->uid;

					//shared_ptr<CMessageBuffer> buf = resolvemsg.serialize();

					// send it
					//boost::system::error_code err;
					//socket.send_to(boost::asio::buffer(buf->getBuffer().at(0).data(), buf->getBuffer().size()), hiimapEndpoint, 0, err);

					//DBG_DEBUG(FMT("No existing locator or locators too old. Request re-sent to (remote) mapping service"));
        			sendLocatorRequest(ev->uid);
        		}
        	} else {
				// reply with locator list

				//ipv4::CLocatorList loclist;
				//loclist.list().push_back(ipv4::CLocatorValue(entry.locators));

				//CSimpleNameAddrMapper::Event_ResolveLocatorResponse* resp =
				//		new CSimpleNameAddrMapper::Event_ResolveLocatorResponse(
				//				ev->name, ev->uid, loclist, this, ev->getFrom());

				shared_ptr<CSimpleNameAddrMapper::Event_ResolveLocatorResponse> resp(
						new CSimpleNameAddrMapper::Event_ResolveLocatorResponse(
								ev->name, ev->uid, entry.locators, this, ev->getFrom()));

				sendMessage(resp);

				//DBG_DEBUG(FMT("Locator request answered using existing mapping entry"));

        	}
        }
        
    } else if (event->getId() == EVENT_REPOSITORY_MULTIPLEXERADDED) {
    	shared_ptr<INetletRepository::Event_MultiplexerAdded> ev =
    			msg->cast<INetletRepository::Event_MultiplexerAdded>();
    	if (ev->archId == "architecture://edu.kit.tm/itm/simpleArch") {
    		multiplexer = (CSimpleMultiplexer*) nodeArch->getMultiplexer(ev->archId);
    		if (multiplexer != NULL) {
    			multiplexer->getNameAddrMapper()->registerListener(EVENT_SIMPLEARCH_RESOLVELOCATORREQ, this);
    			DBG_DEBUG(FMT("%1% registered at name/addr mapper of %2%") %
    					getClassName() % ev->archId);

    		}

    	}

    } else if(event->getId() == EVENT_NETLETSELECTOR_NOSUITABLENETLET) {
    	DBG_DEBUG(FMT("%1% received %2%") % getClassName() % event->getId());
    	shared_ptr<CNetletSelector::Event_NoSuitableNetlet> ev =
    			msg->cast<CNetletSelector::Event_NoSuitableNetlet>();

    	if (ev->appConn->getRemoteURI().compare(0, 8, "video://") == 0) {
    		string m = "This remote object requires connecting to the following network: "
    				"VideoNetwork\n\n"
    				//"Which is part of the following network architecture:\n"
    				//"architecture://edu.kit.tm/itm/simpleArch\n\n"
    				"Do you want to connect and download required protocols?";
    		ev->appConn->sendUserRequest(m,
    				IAppConnector::userreq_yesno,
    				this /* reply to */,
    				1 /* optional request ID */);

    	} else {
    		string m = "No suitable Netlets found for this remote object.";
			ev->appConn->sendUserRequest(m,
					IAppConnector::userreq_cancel,
					this /* reply to */,
					0 /* optional request ID */);

    	}

    } else {
        throw EUnhandledMessage((FMT("Received unknown event message %1%") % msg->getClassName()).str());
    }
}

/**
* @brief Process a timer message directed to this message processing unit
*
* @param msg	Pointer to message
*/
void Bb_HiimapUser::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
    if(msg->cast<PollTimer>()) {
        io_service.poll();

        /*******************
         * Deployment Code
         ******************/

        if (deployment.step == 1) {
        	// we are still waiting for a Network entry

        	// resolve network id in HiiMap
        	sendLocatorRequest(deployment.uid);
        	DBG_DEBUG(FMT("DEPLOYMENT: Step1 - Re-requested Network Entry. Name: %1%") % deployment.name);
        }
        if (deployment.step == 2) {
        	// we are still waiting for Netlet/Multiplexer entries

        	// re-request multiplexer entry if necessary
        	if (deployment.multiplexer.resolved == false) {
        		sendLocatorRequest(deployment.multiplexer.uid);
        		DBG_DEBUG(FMT("DEPLOYMENT: Step2 - Re-requested Multiplexer Entry. Name: %1%") % deployment.multiplexer.name);
        	}

        	// re-request netlet entries if necessary
        	std::list<HiiMapDeployEntry>::iterator netlet;
        	for (netlet=deployment.netlets.begin(); netlet != deployment.netlets.end(); netlet++) {
        		if (netlet->resolved == false) {
        			sendLocatorRequest(netlet->uid);
        			DBG_DEBUG(FMT("DEPLOYMENT: Step2 - Re-requested Netlet Entry. Name: %1%") % netlet->name);
        		}
        	}
        }

        // re-issue timer
        shared_ptr<CTimer> pollTimer(new PollTimer(HIIMAP_POLLTIMER, this));
        scheduler->setTimer(pollTimer);

    } else {
        DBG_ERROR("Unhandled timer!");
        throw EUnhandledMessage();  
    }
}

/**
* @brief Process an outgoing message directed towards the network.
*
* @param msg	Pointer to message
*/
void Bb_HiimapUser::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
    // not used with this BB

    DBG_ERROR("Unhandled outgoing message!");
    throw EUnhandledMessage();

}

/**
* @brief Process an incoming message directed towards the application.
*
* @param msg	Pointer to message
*/
void Bb_HiimapUser::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// not used with this BB

    DBG_ERROR("Unhandled incoming message!");
    throw EUnhandledMessage();
}

}
}
}
