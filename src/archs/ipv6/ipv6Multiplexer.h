/** @file
 * ipv6Multiplexer.h
 *
 * @brief	Multiplexer for IPv6
 *
 * (c) 2012 Institut fuer Telematik, KIT, Germany
 *
 *  Created on: Nov 23, 2012
 *      Author: denis
 */

#ifndef IPV6MULTIPLEXER_H_
#define IPV6MULTIPLEXER_H_

#include "netletMultiplexer.h"
#include "nameAddrMapper.h"
#include "messageBuffer.h"
#include "netAdapt.h"

#include "header_ipv6.h"

#include <sys/types.h>
#include <set>

class INetlet;
class CNetletSelector;

namespace ipv6 {

#define IPV6_ARCH_NAME "architecture://ipv6"

#define EVENT_IPV6_UNHANDLEDPACKET	"event://ipv6/UnhandledPacket"


/* ========================================================================= */

/**
 * @brief 	Name/address mapper
 */
class CIPv6NameAddrMapper : public INameAddrMapper
{
private:
	std::string archName;

public:
	CIPv6NameAddrMapper(const std::string& archName, CNena* nodeA, IMessageScheduler *sched);
	virtual ~CIPv6NameAddrMapper();

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	virtual void getPotentialNetlets(IAppConnector* appc, std::list<INetletMetaData*>& netletList) const;
};

/* ========================================================================= */

/**
 * @brief IPv6 Netlet multiplexer
 */
class CIPv6Multiplexer : public INetletMultiplexer
{
public:
	/**
	 * @brief	Event emitted if a packet for an unknown Netlet is received.
	 *
	 * 			Note, that the packet is only a copy which is destroyed as
	 * 			soon as the event is destroyed.
	 */
	class Event_UnhandledPacket : public IEvent
	{
	public:
		boost::shared_ptr<IMessage> packet;

		Event_UnhandledPacket(
			boost::shared_ptr<IMessage> packet = boost::shared_ptr<IMessage>(),
			IMessageProcessor *from = NULL, IMessageProcessor *to = NULL)
		: IEvent(from, to), packet(packet)
		{
			className += "::Event_UnhandledPacket";
		}

		virtual ~Event_UnhandledPacket()
		{}

		virtual const std::string getId() const
		{
			return EVENT_IPV6_UNHANDLEDPACKET;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			boost::shared_ptr<IEvent> msg(new Event_UnhandledPacket(packet, from, to));
			return msg;
		}
	};

protected:
	std::map<nena::hash_t, INetlet*> hashedNetlets;
	CIPv6NameAddrMapper* nameAddrMapper;

	CNetletSelector* netletSelector;

public:
	CIPv6Multiplexer(IMultiplexerMetaData *metaData, CNena *nodeA, IMessageScheduler *sched);
	virtual ~CIPv6Multiplexer();

	// from INodeArchProcessor

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

	// from IMultiplexer

	/**
	 * @brief	Called if new Netlets of this architecture were added to the system.
	 */
	virtual void refreshNetlets();

	virtual std::string getStats();
};

/* ========================================================================= */

/**
 * @brief Multiplexer meta data class
 *
 * The multiplexer meta data class has two purposes: On one hand, it describes
 * the architecure's properties, on the other hand, it provides a factory
 * functions that will be used by the node architecture daemon to instantiate
 * new "architecture".
 */
class CIPv6MultiplexerMetaData : public IMultiplexerMetaData
{
private:
	std::string archName;

public:
	CIPv6MultiplexerMetaData();
	virtual ~CIPv6MultiplexerMetaData();

	virtual std::string getArchName() const;

	virtual INetletMultiplexer * createMultiplexer(CNena * nodeA, IMessageScheduler *sched); 		///< factory function
};

} // namespace ipv6

#endif /* IPV6MULTIPLEXER_H_ */
