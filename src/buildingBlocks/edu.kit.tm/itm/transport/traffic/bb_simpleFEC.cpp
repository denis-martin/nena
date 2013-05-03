/*
 * bb_simpleFEC.cpp
 *
 *  Created on: Mar 14, 2011
 *      Author: sadik asbach
 */

#include <sys/time.h>
#include "bb_simpleFEC.h"

#include "nodeArchitecture.h"

namespace edu_kit_tm {
namespace itm {
namespace transport {

using std::list;
using std::string;

const string simpleFecClassName = "bb://edu.kit.tm/itm/transport/traffic/simpleFEC";

Bb_SimpleFEC::Bb_SimpleFEC(CNodeArchitecture *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const string id)
	: IBuildingBlock(nodeArch, sched, netlet, id), fecLength(FEC_LENGTH), fecInterleaving(FEC_INTERLEAVING)
{
	className += "::Bb_SimpleFEC";
	
	registerEvent(EVENT_SIMPLEARCH_SIMPLEFEC_PACKETLOSS);
	
	packetNr = 						0;
	fecNr = 						0;
	buffering = 					true;
	bufferPosition = 				0;
	handPackets = 					1;
	adaptFEC = 						false;
	prevPacketLen = (fecInterleaving > 0) ? new unsigned short[fecInterleaving] : NULL;
	
	
//	lastRcvdSec = 0;
//	interPacketDelay = 0;
//	gettimeofday(&rcvdTime, 0);
//	lastTimer = NULL;

	statReceivedPkts = 				0;
	statHandedToAppPkts = 			0;
	statPacketLossNetTimer =		NULL;
	statPacketLossNet = 			0;
	statPacketLossNetBursts =		0;
	statPacketLossApp = 			0;
}

Bb_SimpleFEC::~Bb_SimpleFEC()
{
	clearBuffer();
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleFEC::processEvent(IMessage *msg) throw (EUnhandledMessage)
{
    IEvent *ev = static_cast<IEvent*>(msg);
	if (ev->getId() == EVENT_SIMPLEARCH_SIMPLEFEC_ADAPTFEC) 
	{
	    Event_adaptFEC *evAFEC = static_cast<Event_adaptFEC*>(ev);
	    
	    DBG_DEBUG(FMT("set fec to len %1% and int %2%") % evAFEC->fecLength % evAFEC->fecInterleaving);
	    
	    newFECLength = evAFEC->fecLength;
	    newFECInterleaving = evAFEC->fecInterleaving;
	    adaptFEC = true;
	    
	    delete evAFEC;
	    evAFEC = NULL;
	}
	else
	{
	    DBG_ERROR("Unhandled event!");
	    throw EUnhandledMessage();
	}
	
	ev = NULL;
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleFEC::processTimer(IMessage *msg) throw (EUnhandledMessage)
{
    PacketLossTimer* timerPL = dynamic_cast<PacketLossTimer*>(msg);
    if(timerPL == NULL)
    {
	    //CTimer* timer = dynamic_cast<CTimer*>(msg);
	    //if (timer == NULL) 
	    {
		    DBG_ERROR("Unhandled timer!");
		    throw EUnhandledMessage();
	    }
/*
	    lastTimer = NULL;

	    if(buffer.size() > 0 && !buffering)
	    {
		    DBG_DEBUG("STATUS fec: timer");
		    sendNextPacket();
	    }
	    else
	    {
		    buffering = true;
	    }*/
	}
	
	//notify listeners and reset packLossInterval counter
	notifyListeners(Event_packetLoss(statReceivedPkts, statPacketLossNet, statPacketLossApp, statPacketLossNetBursts, statHandedToAppPkts, statBurstsNet));
	statReceivedPkts = 0;
	statPacketLossNet = 0;
	statPacketLossApp = 0;
	statHandedToAppPkts = 0;
	statPacketLossNetBursts = 0;
	scheduler->setTimer(statPacketLossNetTimer);
	statBurstsNet.clear();
	
	timerPL = NULL;
	msg = NULL;
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleFEC::processOutgoing(IMessage *msg) throw (EUnhandledMessage)
{	
	CPacket *pkt = dynamic_cast<CPacket *> (msg);
	if (pkt == NULL)
	{
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	if(fecInterleaving > 0)
	{
		//fec packet composion
		if(fecNr < fecInterleaving)
		{
			fecPackets[fecNr] = new CPacket(*pkt);
		}
		else
		{
			CBuffer* buf1 = fecPackets[fecNr%fecInterleaving]->getPayload();
			CBuffer* buf2 = pkt->getPayload();

			int size1 = buf1->size();
			int size2 = buf2->size();

			//realloc if new Packet is bigger
			if(size2 > size1)
			{
				buf1->resize(size2);
			}

			doSimpleFEC(&buf1->at(0), &buf2->at(0), size2);
		}
	}

	//send data packet
	SimpleFEC_Header* header = new SimpleFEC_Header();
	
	header->packetNr = packetNr;
	header->fecLength = fecLength;
	header->fecInterleaving = fecInterleaving;
	header->offset = fecNr;	
	//set packet len
	if(fecInterleaving > 0)
	{
		header->prevPacketLen = (fecNr < fecInterleaving) ? 0 : prevPacketLen[fecNr%fecInterleaving];
		prevPacketLen[fecNr%fecInterleaving] = pkt->getPayload()->size();
	}
	else
	{
		header->prevPacketLen = 0;
	}
	pkt->pushHeader(header);

	//send pkt
	pkt->setFrom(this);
	pkt->setTo(getNext());
	sendMessage(pkt);
		
	packetNr++;

	if(fecInterleaving > 0)
	{
		//send fec packets //TODO: better delay them?
		if(fecNr == fecLength * fecInterleaving - 1)
		{
			int i;
			CPacket* fecPkt;
		
			for(i = 0; i < fecInterleaving; i++)
			{
				fecPkt = fecPackets[i];
				
				header = new SimpleFEC_Header();
				header->packetNr = packetNr;
				header->fecLength = fecLength;
		        header->fecInterleaving = fecInterleaving;
		        header->offset = fecNr+(i+1);
		        header->prevPacketLen = prevPacketLen[i];
			
				fecPkt->pushHeader(header);
				fecPkt->setFrom(this);
				fecPkt->setTo(getNext());
				sendMessage(fecPkt);
			
				fecPackets[i] = NULL;
				packetNr++;
			}

			fecNr = 0;
		
			if(adaptFEC)
			{
				fecInterleaving = newFECInterleaving;
				fecLength = newFECLength;
				
				delete prevPacketLen;
				prevPacketLen = (fecInterleaving > 0) ? new unsigned short[fecInterleaving] : NULL;
				
				adaptFEC = false;
			}
		}
		else
		{
			fecNr++;
		}
	}
	else if(adaptFEC)
	{
		fecInterleaving = newFECInterleaving;
		fecLength = newFECLength;
		
		delete prevPacketLen;
		prevPacketLen = (fecInterleaving > 0) ? new unsigned short[fecInterleaving] : NULL;
		
		adaptFEC = false;
	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void Bb_SimpleFEC::processIncoming(IMessage *msg) throw (EUnhandledMessage)
{
	//static FILE *pLogPnr = fopen("packets.log", "w");
	//static list<int> statPnr;
	//static int countPkts = 0;
	
	// dynamic cast only used to detect bugs early
	CPacket *pkt = dynamic_cast<CPacket *> (msg);
	if (pkt == NULL)
	{
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

    //check if timer yet set
    if(statPacketLossNetTimer == NULL)
    {
        statPacketLossNetTimer = new PacketLossTimer(FEC_EVENT_PACKETLOSS_INTERVAL*0.001, this);
        scheduler->setTimer(statPacketLossNetTimer);
    }

    statReceivedPkts++;
	SimpleFEC_Header* header = pkt->popHeader<SimpleFEC_Header>();
	
	/*statPnr.push_back(header->packetNr - packetNr);
	countPkts++;
	if(statPnr.size() == 400)
	{
		//write to file
		std::list<int>::iterator it;
		int c = countPkts - 400;
		
		for(it = statPnr.begin(); it != statPnr.end(); it++)
		{
			c++;
			fprintf(pLogPnr, "%d %d\n", c, *it);
		}
		
		statPnr.clear();
	}*/
	
    // hand to buffer
	if(!buffer.empty())
	{
		if(header->packetNr == packetNr)
		{
			buffer.push_front(QueueItem(pkt, header->fecLength, header->fecInterleaving, header->prevPacketLen, packetNr));
		}
		else if(header->packetNr > packetNr)
		{
			//DBG_DEBUG("STATUS fec: missing packet");
			int diff = header->packetNr - packetNr;
			
			//check if diff smaller than one fec block//TODO:depends on min(buffering packets) = 2 * fec block
			//if(diff < (header->fecLength + 1) * header->fecInterleaving)
			{
				pushElements(diff, header->fecLength, header->fecInterleaving);
				buffer.push_front(QueueItem(pkt, header->fecLength, header->fecInterleaving, header->prevPacketLen, header->packetNr));

				//check if packet is fec packet
				if(header->offset < fecLength*fecInterleaving)
				{
					handPackets += diff;
				}
			}
			/*else
			{	
				//packet too far out of order, clear buffer
				DBG_DEBUG("STATUS fec: clearing buffer due to too new packet");
				clearBuffer();
				buffering = true;
			}*/
			
			//STATISTICS, assuming packet reordering is very unusual, so ignoring late packets
			/*if(diff > 1)
			{
				//increase burst count
			    std::map<short, short>::iterator it;
			    it = statBurstsNet.find(diff);
			    
			    if(it == statBurstsNet.end())
			    {
			        statBurstsNet.insert(std::pair<int, short>(diff, 1));
			    }
			    else
			    {
			        (*it).second++;
			    }
			    statPacketLossNetBursts += diff;
			}*/
	
			statPacketLossNet += diff;
		}
		else
		{
			DBG_DEBUG("STATUS fec: old packet");
			//check if packet in fec block which is in buffer and not actually send (whole block)
			//TODO: if we got new fec parameters, which are not set yet, this may be inconsistent
			if(packetNr - header->packetNr <= (buffer.size() + bufferPosition) - (fecLength + 1) * fecInterleaving)
			{
				//exchange empty packet in buffer
				list<QueueItem>::iterator it = buffer.begin();
				advance(it, (packetNr - header->packetNr) - 1);

				it = buffer.erase(it);
				buffer.insert(it, QueueItem(pkt, header->fecLength, header->fecInterleaving, header->prevPacketLen, header->packetNr));
			}
			else
			{
				//packet too far out of order, clear buffer
				DBG_DEBUG("STATUS fec: clearing buffer due to too old packet");
				clearBuffer();
				buffering = true;
			}
		}
	}
	else if(fecInterleaving == 0 && header->packetNr > packetNr)
	{
		int diff = header->packetNr - packetNr;
		
		if(diff > 1)
		{
			//increase burst count
		    std::map<short, short>::iterator it;
		    it = statBurstsNet.find(diff);
		    
		    if(it == statBurstsNet.end())
		    {
		        statBurstsNet.insert(std::pair<int, short>(diff, 1));
		    }
		    else
		    {
		        (*it).second++;
		    }
		    statPacketLossNetBursts += diff;
		}

		statPacketLossNet += diff;
		statPacketLossApp += diff;
	}
	else if(fecInterleaving == 0 && header->packetNr < packetNr)
	{
		DBG_DEBUG(FMT("FEC: got old packet, diff is %1%") % (packetNr - header->packetNr));
	}

	if(buffer.empty())
	{
	    //push missing packets from actual fec block
		if(header->offset > 0)
		{
			pushElements(header->offset, header->fecLength, header->fecInterleaving);
		}
		
		//push packet
		buffer.push_front(QueueItem(pkt, header->fecLength, header->fecInterleaving, header->prevPacketLen, header->packetNr));
	}

	packetNr = header->packetNr + 1;
	
	//send if not buffering and packet is not fec packet
	if(!buffering && (fecInterleaving == 0 || packetNr % ((fecLength+1)*fecInterleaving) < fecLength*fecInterleaving))
	{
		for(short i = 0; i < handPackets; i++)
		{
			sendNextPacket();
		}
			
		handPackets = 1;
	}
	//start sending to application if buffer full
	else if(buffering && FEC_BUFFER_SIZE <= buffer.size())
	{
		buffering = false;
		handPackets = 1;
		
		sendNextPacket();
	}
	/*else if(buffering)
	{
		DBG_DEBUG(FMT("%1% packets in buffer") % buffer.size());
	}*/
}


/**
 * @brief Push x NULL elements into buffer
 *
 * @param x		number of elements to push
 * @param length        fecLength of FEC block the packet is belonging to
 * @param interleaving  interleaving of FEC block the packet is belonging to
 */
inline void Bb_SimpleFEC::pushElements(short x, short length, short interleaving)
{
	short i = 0;
	for(; i < x; i++)
	{
		buffer.push_front(QueueItem(length, interleaving));
	}
}


/**
 * @brief clear the buffer
 */
inline void Bb_SimpleFEC::clearBuffer()
{
	short i;
	CPacket* pkt;
	
	while(buffer.size()>0)
	{
		pkt = buffer.back().pkt;
		if(pkt != NULL)
		{
			delete pkt;
		}
		buffer.pop_back();
	}

	buffering = true;
}


/**
 * @brief xor's the payload of two packets into packet 1
 *
 * @param pkt1	packet 1
 * @param pkt2	packet 2
 * @param len   size of pkt2
 */
inline void Bb_SimpleFEC::doSimpleFEC(unsigned char* buf1, unsigned char* buf2, int len)
{
	//generic fec (xor)
	for(int i = 0; i < len; i++, buf1++, buf2++)
	{
		*buf1 ^= *buf2;
	}
}


void Bb_SimpleFEC::sendNextPacket()
{
	CPacket* pkt;
	//DBG_DEBUG("STATUS fec: send next");
	
	//if we are at pos 0 (fec block beginning) we can check if block complete and if there are any missing packets in
	if(bufferPosition == 0 && fecInterleaving > 0)
	{
	    list<QueueItem>::iterator buffer_it = buffer.end();
	    buffer_it--;
	    
	    //check if fecLength or fecInterleaving changed
	    if((*buffer_it).fecLength != fecLength || (*buffer_it).fecInterleaving != fecInterleaving)
	    {
	        //set new fec params
	        fecLength = (*buffer_it).fecLength;
	        fecInterleaving = (*buffer_it).fecInterleaving;

	        //DBG_DEBUG("!!!!!!!!!!!parameter changed!!!!!!!!!");
	    }
	    
	    //check if complete fec Block in buffer, else we won't perform fec
	    if(buffer.size() >= (fecLength+1)*fecInterleaving)
	    {
		    //check if we need to perform fec (packet(s) missing)
		    unsigned short *count = new unsigned short[fecInterleaving];
		    unsigned short len;
		    bool doFEC = false;
		    
		    //init count with null
		    for(len = 0; len < fecInterleaving; len++)
		    {
		    	count[len] = 0;
		    }
		    
		    //walk through fec block
		    for(len = (fecLength+1)*fecInterleaving; bufferPosition < len; bufferPosition++, buffer_it--)
		    {
		        //check if packet NULL
			    if((*buffer_it).pkt == NULL)
			    {
			        //check if packet "not an FEC packet" (they are the last #fecInterleaving packets)
			        //or "FEC packet and yet packet(s) missing" (because we dont need to reconstruct
			        //fec packets but have to count them if yet a packet is missing)
			        if(bufferPosition < fecLength*fecInterleaving || count[bufferPosition%fecInterleaving] > 0)
			        {
				        count[bufferPosition%fecInterleaving]++;
				        doFEC = true;
				    }
			    }
		    }

		    //perform fec if max 1 packet per interleaving-row missing
		    if(doFEC)
		    {
			    unsigned short i, j;
			
			    CSerialBuffer *CSBuf1, *CSBuf2;
			    unsigned char *buf1, *buf2;
			    unsigned short packetLen;
			    int size1, size2;
			    CPacket* pktReconstructed;
			    CPacket** pktReconstruct;
			
			    for(i = 1; i <= fecInterleaving; i++)
			    {
			        //check if max 1 packet is missing
				    if(count[i-1] == 1)
				    {
					    pktReconstructed = NULL;
					    pktReconstruct = NULL;
					    buffer_it = buffer.end();
					    advance(buffer_it, -1*i);
					    packetLen = 0;

					    for(j = 0; j <= fecLength; j++, advance(buffer_it, -1*fecInterleaving))
					    {
						    pkt = (*buffer_it).pkt;
						    if(pkt == NULL)
						    {
							    pktReconstruct = &(*buffer_it).pkt;
							    
							    //get lost packet's size from next packet in fec block
							    advance(buffer_it, -1*fecInterleaving);
							    packetLen = (*buffer_it).prevPacketLen;
							    advance(buffer_it, 1*fecInterleaving);
						    }
						    else if(pktReconstructed == NULL)
						    {
							    pktReconstructed = new CPacket(*pkt);
						    }
						    else
						    {
							    CSBuf1 = pktReconstructed->getSerialBuffer();
							    CSBuf2 = pkt->getSerialBuffer();

							    buf1 = CSBuf1->getRemainingBuffer();
							    buf2 = CSBuf2->getRemainingBuffer();
	
							    size1 = CSBuf1->getRemainingSize();
							    size2 = CSBuf2->getRemainingSize();

							    if(size2 > size1)
							    {
								    CSBuf1->resize(CSBuf1->size() + (size2 - size1));
							    }

							    doSimpleFEC(buf1, buf2, size2);
						    }
					    }

						//set len of packet
						CSBuf1 = pktReconstructed->getSerialBuffer();
						if(CSBuf1->getRemainingSize() > packetLen)
						{
							CSBuf1->resize(CSBuf1->size() - (CSBuf1->getRemainingSize() - packetLen));
						}
						else if(CSBuf1->getRemainingSize() < packetLen)
						{
							DBG_DEBUG("ERROR: Size of received packets too small");
						}

					    *pktReconstruct = pktReconstructed;
				    }
				   /* else if(count[i-1]>0){
				    
				    buffer_it = buffer.end();
	   				buffer_it--;
				    bufferPosition=0;
				    for(len = (fecLength+1)*fecInterleaving; bufferPosition < len; bufferPosition++, buffer_it--)
					{
						//check if packet NULL
						if((*buffer_it).pkt != NULL)
						{
							DBG_DEBUG(FMT("%1%: %2%")%bufferPosition%(*buffer_it).pnr);
						}
						else
						{
							DBG_DEBUG(FMT("%1%: NULL")%bufferPosition);
						}
					}
				    
				    }*/
			    }
		    }
		
		    delete [] count;
		    count = NULL;
	
		    bufferPosition = 0;
		}
		else
		{
			//buffer to small to perform fec -> buffering and keep packets in buffer
			//TODO: what will happen with the last few elements?
			buffering = true;
			return;
		}
	}

	// hand to upper entity

	if(buffer.back().pkt != NULL)
	{
		pkt = buffer.back().pkt;
		pkt->setFrom(this);
		pkt->setTo(getPrev());
		sendMessage(pkt);
		
		pkt = NULL;
		buffer.back().pkt = NULL;
		
		statHandedToAppPkts++;
	}
	else
	{
	    //packet could not be reconstructed
		statPacketLossApp++;
	}
	buffer.pop_back();

	if(fecInterleaving > 0)
	{
		bufferPosition++;

		if(bufferPosition == fecLength*fecInterleaving)
		{
			//discard fec packets
			for(;bufferPosition < (fecLength+1)*fecInterleaving && !buffer.empty(); bufferPosition++)
			{
				pkt = buffer.back().pkt;
				if(pkt != NULL)
				{
					delete pkt;
					pkt = NULL;
					buffer.back().pkt = NULL;
				}
				buffer.pop_back();
			}

			bufferPosition = 0;
		}
	
		if(buffer.size() > 0)
		{
			//lastTimer = new CTimer(interPacketDelay, this);
			//scheduler->setTimer(lastTimer);
		}
		else
		{
			buffering = true;
		}
	}
	else if(buffer.back().pkt != NULL)
	{
		//check for new fec parameters
	    if(buffer.back().fecLength != fecLength || buffer.back().fecInterleaving != fecInterleaving)
	    {
	        //set new fec params
	        fecLength = buffer.back().fecLength;
	        fecInterleaving = buffer.back().fecInterleaving;
	    }
	}
}

}
}
}
