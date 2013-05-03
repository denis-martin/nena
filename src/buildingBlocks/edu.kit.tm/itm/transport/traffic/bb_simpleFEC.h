/*
 * bb_simpleFEC.h
 *
 * fec packet: the generic composed packet
 * fec block:  FEC_LENGTH*FEC_INTERLEAVING "packets" + FEC_INTERLEAVING "fec packets"
 *
 *  Created on: Mar 14, 2011
 *      Author: sadik asbach
 */

#ifndef BB_SIMPLEFEC_H_
#define BB_SIMPLEFEC_H_

#include "composableNetlet.h"

#include "packets.h"

namespace edu_kit_tm {
namespace itm {
namespace transport {

#define EVENT_SIMPLEARCH_SIMPLEFEC_PACKETLOSS  "event://simpleArch/simpleFec/packetLoss"
#define EVENT_SIMPLEARCH_SIMPLEFEC_ADAPTFEC  "event://simpleArch/simpleFec/adaptFec"

#define FEC_LENGTH 						1			    //packets composed to one fec packet, > 0
#define FEC_INTERLEAVING 				0			    //interleaving off(1)/on(2,3,..), 0 = no FEC at all
#define FEC_INTERLEAVING_MAX 			32              //max interleaving
#define FEC_BUFFER_SIZE 				100			    //packet buffer size, asserting >= (FEC_LENGTH + 1) * FEC_INTERLEAVING
#define FEC_MOVING_AVARAGE_ORDER 		4		        //order of moving avarage to calc mean inter packet delay, HAS TO BE > 1
#define FEC_EVENT_PACKETLOSS_INTERVAL 	1000 		    //interval(ms) the packetloss event is fired at

/**
 * @brief Minimum header containing a hash of the application's service ID
 */
class SimpleFEC_Header: public IHeader
{
public:
	uint32_t packetNr;
	short fecLength;
	short fecInterleaving;
	short offset;
	unsigned short prevPacketLen;

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 *
	 * Header format is [AAAA]
	 * A is the local connection id
	 */
	virtual void serialize(CSerialBuffer *buffer)
	{
		assert(buffer != NULL);
		buffer->push_ulong((uint32_t) packetNr);
		buffer->push_uchar((unsigned char) fecLength);
		buffer->push_uchar((unsigned char) fecInterleaving);
		buffer->push_uchar((unsigned char) offset);
		buffer->push_ushort(prevPacketLen);
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 *
	 * Header format is [AAAA]
	 * A is the local connection id
	 */
	virtual void deserialize(CSerialBuffer *buffer)
	{
		assert(buffer != NULL);
		packetNr = buffer->pop_ulong();
		fecLength = (short) buffer->pop_uchar();
		fecInterleaving = (short) buffer->pop_uchar();
		offset = (short) buffer->pop_uchar();
		prevPacketLen = buffer->pop_ushort();
	}

	/**
	 * @brief Return a copy of the header
	 */
	virtual IHeader* clone() const
	{
		return (IHeader*) new SimpleFEC_Header(*this);
	}

};

extern const std::string simpleFecClassName;

class Bb_SimpleFEC: public IBuildingBlock
{
public:
    /**
	 * @brief	Event emitted all FEC_EVENT_PACKETLOSS_INTERVAL ms containing information about actual packetloss
	 *
	 */
	class Event_packetLoss : public IEvent
	{
	public:
	    unsigned short interval;            		//interval the event is fired at in ms
		unsigned short receivedPkts;				//received packets in interval
		unsigned short handedToAppPkts;				//handed to app in interval
	    unsigned short packetLossNet;  				//packets lost in last interval from net
	    unsigned short packetLossApp;			 	//not reconstructable Packets
		unsigned short packetLossNetBursts;			//packets lost in bursts
		std::map<short, short> *bursts;				//map of bursts (net)
	    	
		Event_packetLoss(   unsigned short rP, unsigned short pLN, unsigned short pLA, unsigned short pLB, unsigned short hAP,
							const std::map<short, short> &b, unsigned short Interval = FEC_EVENT_PACKETLOSS_INTERVAL) : IEvent()
		{
			className += "::Event_packetLoss";
			
			interval = Interval;
			receivedPkts = rP;
			packetLossNet = pLN;
			packetLossApp = pLA;
			packetLossNetBursts = pLB;
			handedToAppPkts = hAP;
			bursts = new std::map<short, short> (b);
		}

		virtual ~Event_packetLoss() {}

		virtual const std::string getId() const
		{
			return EVENT_SIMPLEARCH_SIMPLEFEC_PACKETLOSS;
		}

		virtual IEvent* clone() const
		{
			return (IEvent*) new Event_packetLoss (receivedPkts, packetLossNet, packetLossApp, packetLossNetBursts, handedToAppPkts, *bursts, interval);
		}
	};
	
	/**
	 * @brief	Event which is listened to, to change fecLength and fecInterleaving.
	 *          registerEvent and registerListener have to be called from the invoking netlet/bb
	 */
	class Event_adaptFEC : public IEvent
	{
	public:
	    short fecLength;           //fec length
	    short fecInterleaving;     //interleaving
	    	    	
		Event_adaptFEC(short length, short interleaving) : IEvent()
		{
			className += "::Event_adaptFEC";
			
			fecLength = length;
			fecInterleaving = interleaving;
		}

		virtual ~Event_adaptFEC() {}

		virtual const std::string getId() const
		{
			return EVENT_SIMPLEARCH_SIMPLEFEC_ADAPTFEC;
		}

		virtual IEvent* clone() const
		{
			return (IEvent*) new Event_adaptFEC (fecLength, fecInterleaving);
		}
	};

private:
	uint32_t packetNr;              //packetNr is for outgoing the continous packet numeration, for incoming the next expected packet nr
	short fecLength;				//actual fecLength
	short fecInterleaving;			//actual fecInterleaving
	
	class QueueItem
	{
	public:
		CPacket* pkt;
		unsigned short fecLength;
		unsigned short fecInterleaving;
		unsigned short prevPacketLen;
		int pnr;

		QueueItem(unsigned short length, unsigned short interleaving) : pkt(NULL) , fecLength(length), fecInterleaving(interleaving), pnr(0), prevPacketLen(0) {}
		QueueItem(CPacket* pkt, unsigned short length, unsigned short interleaving, unsigned short pPL, int pn) : pkt(pkt),
		                    fecLength(length), fecInterleaving(interleaving), prevPacketLen(pPL), pnr(pn) {}
		virtual ~QueueItem() {}
	};
	
	class PacketLossTimer : public CTimer
	{
	public:
	    PacketLossTimer(double timeout, IMessageProcessor *proc) : CTimer(timeout, proc) {}
	    
	    virtual ~PacketLossTimer() {}
	};

	//outgoing
	short fecNr;                            //nr of current packet in current fec block
	CPacket *fecPackets[FEC_INTERLEAVING_MAX]; //holds pointers to the (to be) composed fec packets
	short newFECLength;                     //to be set as fecLength
	short newFECInterleaving;               //to be set as fecInterleaving
	bool adaptFEC;                          //set new fecLength and fecInterleaving
	unsigned short *prevPacketLen;			//array containing the packet lengths
	
	//incoming
	std::list<QueueItem> buffer;            //buffer for incoming packets, tail is the first received packet in buffer
	bool buffering;                         //indicates buffering
//	timeval rcvdTime;
//	double lastRcvdSec;
//	double interPacketDelays[FEC_MOVING_AVARAGE_ORDER];
//	double interPacketDelay;
//	CTimer* lastTimer;
	unsigned short bufferPosition; 		    //position in current fecblock, always < fec block size ((FEC_LENGTH+1)*FEC_INTERLEAVING)
	short handPackets;                      //count of packets to be handed to app in next sendNextPacket step
    
    //statistics (incoming)
	unsigned short statReceivedPkts;				//received packets in interval
	unsigned short statHandedToAppPkts;				//packets handed to app
    unsigned short statPacketLossNet;      			//packets lost in last interval from net
    unsigned short statPacketLossApp;      			//packets lost in last interval to app
    unsigned short statPacketLossNetBursts;			//in bursts lost packets
    PacketLossTimer* statPacketLossNetTimer;       	//timer to invoke packetloss event
    std::map<short, short> statBurstsNet;	   		//map representing <burstlength,count>

	
public:
	Bb_SimpleFEC(CNodeArchitecture *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_SimpleFEC();

	// from IMessageProcessor

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(IMessage *msg) throw (EUnhandledMessage);

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(IMessage *msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(IMessage *msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an incoming message directed towards the application.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processIncoming(IMessage *msg) throw (EUnhandledMessage);

	// from IBuildingBlock

	virtual Id getId() const;

	/**
	 * @brief Push x NULL elements into buffer
	 *
	 * @param x		        number of elements to push
	 * @param length        fecLength of FEC block the packet is belonging to
	 * @param interleaving  interleaving of FEC block the packet is belonging to
	 */
	void pushElements(short x, short length, short interleaving);

	/**
	 * @brief clear the buffer
	 */
	void clearBuffer();

	/**
	 * @brief sends next packet
	 */
	void sendNextPacket();

	/**
	 * @brief xor's the payload of two packets into packet 1
	 *
	 * @param pkt1	packet 1
	 * @param pkt2	packet 2
	 * @param len   size of pkt2
	 */
	void doSimpleFEC(unsigned char* buf1, unsigned char* buf2, int len);
};

} // transport
} // itm
} // edu.kit.tm

#endif /* BB_SIMPLEFEC_H_ */
