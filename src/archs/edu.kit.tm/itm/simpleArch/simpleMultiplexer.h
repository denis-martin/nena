/** @file
 * simpleMultiplexer.h
 *
 * @brief	Multiplexer for "Simple Architecture".
 *
 * The Simple Architecture has the following features:
 * - A simple transport Netlet. At the moment, it just hands the application's
 *   data to the multiplexer.
 * - A simple routing Netlet. A very dumb one, just creating a forwarding
 *   information base (FIB).
 * - A forwarding mechanism based on the FIB. This mechanism is realized within
 *   the multiplexer. Note, that other architectures may do the forwarding in
 *   a Netlet.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#ifndef SIMPLEMULTIPLEXER_H_
#define SIMPLEMULTIPLEXER_H_

#include "netletMultiplexer.h"
#include "nameAddrMapper.h"
#include "messageBuffer.h"
#include "netAdapt.h"

// yes, we're based on IPv4
#include "archdep/ipv4.h"

#include <sys/types.h>
#include <set>

class INetlet;
class CNetletSelector;

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

#define SIMPLE_ARCH_NAME "architecture://edu.kit.tm/itm/simpleArch"

#define EVENT_SIMPLEARCH_UNHANDLEDPACKET	"event://simpleArch/UnhandledPacket"

#define EVENT_SIMPLEARCH_RESOLVELOCATORREQ	"event://simpleArch/nameAddrMapper/ResolveLocatorRequest"
#define EVENT_SIMPLEARCH_RESOLVELOCATORRESP	"event://simpleArch/nameAddrMapper/ResolveLocatorResponse"

class SimpleFib_Entry
{
public:
	ipv4::CLocatorValue destLoc;
	ipv4::CLocatorValue nextHopLoc;
	INetAdapt* netAdapt;
	int hops;
	double timeStamp;

	SimpleFib_Entry():
		netAdapt(NULL), hops(-1), timeStamp(0)
	{
	};

	SimpleFib_Entry(
		const ipv4::CLocatorValue& destLoc,
		const ipv4::CLocatorValue& nextHopLoc = ipv4::CLocatorValue(),
		INetAdapt* netAdapt = NULL,
		int hops = -1,
		double timeStamp = 0):
		destLoc(destLoc), nextHopLoc(nextHopLoc), netAdapt(netAdapt),
		hops(hops), timeStamp(timeStamp)
	{
	};
};

typedef std::map<ipv4::CLocatorValue, SimpleFib_Entry> SimpleFib;	///< locator -> network access

/* ========================================================================= */

/**
 * @brief Minimum header containing a hash of the application's service ID
 */
class SimpleNetlet_Header: public IHeader
{
public:
	nena::hash_t serviceHash;

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 *
	 * Header format is [AAAA]
	 * A is the local connection id
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		std::size_t s = sizeof(uint32_t);
		boost::shared_ptr<CMessageBuffer> mbuf(new CMessageBuffer(s));
		mbuf->push_ulong((uint32_t) serviceHash);
		return mbuf;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 *
	 * Header format is [AAAA]
	 * A is the local connection id
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> mbuf)
	{
		assert(mbuf.get() != NULL);
		serviceHash = (nena::hash_t) mbuf->pop_ulong();
	}

};

/* ========================================================================= */

/**
 * @brief 	Name/address mapper implementation for SimpleArchitecture
 */
class CSimpleNameAddrMapper : public INameAddrMapper
{
public:
	class Event_ResolveLocatorRequest : public IEvent
	{
	public:
		std::string name;
		shared_buffer_t uid;

		Event_ResolveLocatorRequest(
			std::string name = std::string(),
			shared_buffer_t uid = shared_buffer_t(),
			IMessageProcessor *from = NULL,
			IMessageProcessor *to = NULL)
			: IEvent(from, to), name(name), uid(uid)
		{
			className += "::Event_ResolveLocatorRequest";
		}

		virtual const std::string getId() const
		{
			return EVENT_SIMPLEARCH_RESOLVELOCATORREQ;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			boost::shared_ptr<IEvent> msg(new Event_ResolveLocatorRequest(name, uid, from, to));
			return msg;
		}
	};

	class Event_ResolveLocatorResponse : public IEvent
	{
	public:
		std::string name;
		shared_buffer_t uid;
		ipv4::CLocatorList locators;

		Event_ResolveLocatorResponse(
			std::string name = std::string(),
			shared_buffer_t uid = shared_buffer_t(),
			ipv4::CLocatorList locators = ipv4::CLocatorList(),
			IMessageProcessor *from = NULL, IMessageProcessor *to = NULL)
		: IEvent(from, to), name(name), uid(uid), locators(locators)
		{
			className += "::Event_ResolveLocatorResponse";
		}

		virtual const std::string getId() const
		{
			return EVENT_SIMPLEARCH_RESOLVELOCATORRESP;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			boost::shared_ptr<IEvent> msg(new Event_ResolveLocatorResponse(name, uid, locators, from, to));
			return msg;
		}
	};

private:
	std::map<std::string, ipv4::CLocatorList> resolveMap;
	std::string archName;

public:
	CSimpleNameAddrMapper(const std::string& archName, CNena* nodeA, IMessageScheduler *sched);
	virtual ~CSimpleNameAddrMapper();

	void loadResolverConfig();

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	virtual void getPotentialNetlets(IAppConnector* appc, std::list<INetletMetaData*>& netletList) const;

	virtual ipv4::CLocatorValue resolve(const std::string& name);
	virtual std::string resolve(ipv4::CLocatorValue loc);

	virtual ipv4::CLocatorList resolveAll(const std::string& name);

	virtual shared_buffer_t resolveId(const std::string& name);

	virtual std::set<std::string> getURLs ();

	virtual void addBroadcast (ipv4::CLocatorValue loc);

	virtual void addLocator (std::string name, ipv4::CLocatorValue loc);
};

/* ========================================================================= */

/**
 * @brief Simple Netlet multiplexer
 */
class CSimpleMultiplexer : public INetletMultiplexer
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
			return EVENT_SIMPLEARCH_UNHANDLEDPACKET;
		}

		virtual boost::shared_ptr<IEvent> clone() const
		{
			boost::shared_ptr<IEvent> msg(new Event_UnhandledPacket(packet, from, to));
			return msg;
		}
	};

protected:
	std::map<nena::hash_t, INetlet*> hashedNetlets;
	CSimpleNameAddrMapper* nameAddrMapper;

	CNetletSelector* netletSelector;

	SimpleFib fib; 			///< Forwarding information base; TODO: access to this should be thread-safe...

	bool reactiveRouting;	///< true, if we have a reactive routing protocol

	ipv4::CLocatorValue localAddr;	///< transitional

public:
	CSimpleMultiplexer(IMultiplexerMetaData *metaData, CNena *nodeA, IMessageScheduler *sched);
	virtual ~CSimpleMultiplexer();

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

	// own

	virtual SimpleFib& getFib();
	virtual void dbgPrintFib();

	/**
	 * @brief	Adds a FIB entry.
	 */
	virtual void addFibEntry(ipv4::CLocatorValue, ipv4::CLocatorValue, int, INetAdapt* na);

	/**
	 * @brief	Adds a FIB entry.
	 */
	virtual void addFibEntry(const SimpleFib_Entry& entry);

	/**
	 * @brief	Delete a FIB entry.
	 */
	virtual void delFibEntry(ipv4::CLocatorValue);

	/**
	 * @brief	Invalidate the FIB completely.
	 */
	virtual void delFib();

	/**
	 * @brief	Update the timestamp for a FIB entry.
	 */
	virtual void updateTimeStamp(ipv4::CLocatorValue);

	/**
	 * @brief	Returns the timestamp of a FIB entry.
	 */
	virtual double getTimeStamp(ipv4::CLocatorValue);

	/**
	 * @brief	Enables or disables reactive routing support.
	 */
	virtual void setReactiveRoutingEnabled(bool enabled);

	virtual CSimpleNameAddrMapper* getNameAddrMapper() const;	///< return name/addr mapper

	/**
	 * @brief return an xml string containing stats
	 */
	virtual std::string getStats ();
};

/* ========================================================================= */

/**
 * @brief Simple multiplexer meta data class
 *
 * The multiplexer meta data class has two purposes: On one hand, it describes
 * the architecure's properties, on the other hand, it provides a factory
 * functions that will be used by the node architecture daemon to instantiate
 * new "architecture".
 */
class CSimpleMultiplexerMetaData : public IMultiplexerMetaData
{
private:
	std::string archName;

public:
	CSimpleMultiplexerMetaData(const std::string& archName);
	virtual ~CSimpleMultiplexerMetaData();

	virtual std::string getArchName() const;

	virtual INetletMultiplexer * createMultiplexer(CNena * nodeA, IMessageScheduler *sched); 		///< factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLEMULTIPLEXER_H_ */
