/** @file
 * vid_serializer.h
 *
 * @brief	Handles the (de)serialization of video data to/from packets.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Sep 29, 2009
 *      Author: PCB
 */

#ifndef VID_SERIALIZER_H_
#define VID_SERIALIZER_H_

#include "vid_structs.h"

#include <string>

#define IVID_BUFFER_MAX_SIZE	4

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividSerializerClassName;

/**
 * @brief	Building block (de)serializaing video data to/from packets.
 */
class IVid_Serializer : public IVid_Component {

public:

	IVid_Serializer(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id, int fec_levels);
	virtual ~IVid_Serializer();
	
	void setAggregationSize(int size);
	

	// from IVid_Component
	
	virtual int processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	virtual void setFecLevels(int levels);


protected:

	typedef struct {
		unsigned short frame_no;
		unsigned char block_x;
		unsigned char block_y;
		unsigned short block_index;
		unsigned char block_count;
		unsigned char quantization_levels;
		
	} __attribute__((packed)) PacketHeader;

	boost::shared_ptr<CVideoFrame> incoming_frame_buffer[IVID_BUFFER_MAX_SIZE];
	boost::shared_ptr<CVideoFrame> outgoing_frame_buffer[IVID_BUFFER_MAX_SIZE];

	boost::shared_ptr<CVideoFrame> bufferNewIncomingFrame(message_t payload, const std::string& serviceId);
	int bufferOutgoingFrame(boost::shared_ptr<CVideoFrame> frame);
	
	int rlEncodeBlock(int *blk, char *buffer);
	int rlDecodeBlock(const char *buffer, int *blk);
	int rlSkipBlock(char *buffer);

	int buffer_size;
	int aggregate_size;
	
};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividSerializerNofecClassName;

class CVid_Serializer_NoFEC : public IVid_Serializer {

public:

	CVid_Serializer_NoFEC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id) :
		IVid_Serializer(nodeArch, sched, netlet, id, 0)
	{
	}
	
};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividSerializerLofecClassName;

class CVid_Serializer_LoFEC : public IVid_Serializer {

public:

	CVid_Serializer_LoFEC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id) :
		IVid_Serializer(nodeArch, sched, netlet, id, 1)
	{
	}
	
};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividSerializerHifecClassName;

class CVid_Serializer_HiFEC : public IVid_Serializer {

public:

	CVid_Serializer_HiFEC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id) :
		IVid_Serializer(nodeArch, sched, netlet, id, 2)
	{
	}
	
};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

#endif /* VID_SERIALIZER_H_ */
