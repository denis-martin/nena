/*
* bb_hiimapUser.h
*
*  Created on: Jan 21, 2010
*      Author: hans
*/

#ifndef BB_HIIMAPUSER_H_
#define BB_HIIMAPUSER_H_

#include "composableNetlet.h"
#include "netletSelector.h"
#include "messageBuffer.h"
#include "messages.h"

#include "nena.h"

#include "archdep/ipv4.h"

/// Boost libs
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#define BOOSTUDP_RECVBUFSIZE    2048        ///< receive_from buffer size for a single datagram
#define HIIMAP_PORT             50005
#define HIIMAP_LOCALPORT	50006

#define HIIMAP_POLLTIMER		0.01		// <- poll timer interval for io_event
#define HIIMAP_ENTRYTIMEOUT		15			// <- Aging timer for HiiMap entries in local mapping cache
#define HIIMAP_REQTIMEOUT		1			// <- Request re-send timeout

//namespace ipv4 {
//class CLocatorValue;
//class CLocatorList;
//}

namespace edu_kit_tm {
namespace itm {

// forward declaration
namespace simpleArch {
class CSimpleMultiplexer;
}


namespace sig {

typedef shared_buffer_t	hiimap_uid_t;

extern const std::string hiimapUserClassName;

class Bb_HiimapUser: public IBuildingBlock
{
    public:
        Bb_HiimapUser(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
        virtual ~Bb_HiimapUser();

        // from IMessageProcessor

        /**
		 * @brief Process a message directed to this message processing unit
		 *
		 * @param msg	Pointer to message
		 */
		virtual void processMessage(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

        /**
        * @brief Process an event message directed to this message processing unit
        *
        * @param msg	Pointer to message
        */
        virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

        /**
        * @brief Process a timer message directed to this message processing unit
        *
        * @param msg	Pointer to message
        */
        virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

        /**
        * @brief Process an outgoing message directed towards the network.
        *
        * @param msg	Pointer to message
        */
        virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

        /**
        * @brief Process an incoming message directed towards the application.
        *
        * @param msg	Pointer to message
        */
        virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

    private:
        class PollTimer : public CTimer
        {
            public:
                /**
                * @brief Constructor.
                *
                * @param timeout   Timeout in seconds
                * @param proc      Node arch entity the event is linked to (default = NULL)
                */
                PollTimer(double timeout, IMessageProcessor *proc);
                ~PollTimer();
        };

        // Class for HiiMap Mapping entries
        class HiiMapEntry
        {
        public:
        	std::string name; // Not good. Multiple Names using same UID not considered here;
        	hiimap_uid_t uid; // Unique Identifier
        	ipv4::CLocatorList locators; // locator list of the uid
        	//std::string locators;
        	double timestamp; // last resolve time of the uid
        	double lastRequest; // timestamp of the last request sent to the mapping system
        	std::list<IMessageProcessor*> issuers; //who issued query requests?
        	//TODO: consider different UID requests that get hashed into the same mapping entry
        };

        class HiiMapDeployEntry
        {
        public:
        	std::string name; // name of the entry
        	hiimap_uid_t uid; // uinque id of the entry
        	std::string locator; // locator string of the entry
        	std::list<std::string> options; // list of option strings of the entry
        	bool resolved;
        };

        class HiiMapDeploy
        {
        public:
        	uint32_t step; // step of the deployment process
        	std::string name; // name of the network
        	hiimap_uid_t uid; // unique id of the network
        	IAppConnector* appCon; //app connector which triggered the deployment
        	HiiMapDeployEntry multiplexer; // multiplexer used in the network
        	std::list<HiiMapDeployEntry> netlets; // netlets used in the network
        };

        edu_kit_tm::itm::simpleArch::CSimpleMultiplexer* multiplexer;
        INetletRepository* repository;
        CNetletSelector* netletSelector;

        /// io service which does the work for us
        boost::asio::io_service io_service;
        /// socket we want to use
        boost::asio::ip::udp::socket socket;
        /// this is where the last message came from
        boost::asio::ip::udp::endpoint sender;
        /// hiimap endpoint
        boost::asio::ip::udp::endpoint hiimapEndpoint;
        bool hiimapConnected;
        /// incoming buffer
        unsigned char recv_buffer[BOOSTUDP_RECVBUFSIZE];
        /// hiimap port
        int hiimapPort;

        // data structure for locator requests and responses
        std::map<hiimap_uid_t, HiiMapEntry> mappings;

        // data structure for the deployment process TODO: allow multiple deployments at the same time
        HiiMapDeploy deployment;

        /// callback method for socket
        void start_receive();

        void sendLocatorRequest(hiimap_uid_t uid);

        void downloadComponents();

	void sendDeployErrorToApp(std::string m);

        /**
        * @brief callback method for socket
        *
        * We use async receive and call "handle_receive", when something was received.
        *
        */
        void handle_receive(const boost::system::error_code& error, std::size_t /*bytes_transferred*/);

        std::string hiimap_uid_to_str(hiimap_uid_t uid, bool pretty = false, std::size_t count = 0)
        {
			boost::format fmt("%1$02x");
			std::string s;
			std::size_t max = count > 0 ? count : uid.size();
			for (std::size_t i = 0; i < max; i++) {
				s += (fmt % (unsigned int) uid[i]).str();
				if (pretty) {
					if (!((i+1) % 4)) s += " ";
					if (!((i+1) % 32)) s += "\n";
				}
			}
			return s;
        }
};

} // sig
} // itm
} // edu.kit.tm

#endif /* BB_DAEMONSTATUS_H_ */
