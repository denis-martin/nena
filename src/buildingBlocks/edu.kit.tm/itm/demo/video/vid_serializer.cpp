/** @file
 * vid_serializer.cpp
 *
 * @brief	Handles the (de)serialization of video data to/from packets.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 * Compile this one with '-O2 -funroll-loops' !
 *
 *  Created on: Sep 28, 2009
 *      Author: PCB
 */

#include "vid_serializer.h"

#include "debug.h"

using namespace std;

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

const string ividSerializerClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Serializer";

IVid_Serializer::IVid_Serializer(
	CNena *nodeArch,
	IMessageScheduler *sched,
	IComposableNetlet *netlet,
	const std::string id,
	int fec_levels

) : IVid_Component(nodeArch, sched, netlet, id) {

	className += "::CVid_Serializer";

	buffer_size = fec_levels + 1;
	assert(buffer_size < IVID_BUFFER_MAX_SIZE);
	
	aggregate_size = 1024;
	
}

IVid_Serializer::~IVid_Serializer() {

}

void IVid_Serializer::setAggregationSize(int size) {
	aggregate_size = size;
	
}

/**
 * @brief Serialize an outgoing frame 
 *
 * @param frame	Pointer to video frame
 */
int IVid_Serializer::processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame) {
	
	// the number of quantization levels encoded in a packet is limited
	// by the number of buffered frames and the number of quantization
	// levels available for a frame. note that the frame buffer MUST (!)
	// be flushed when changing the FEC level...

	int frames_in_buffer = bufferOutgoingFrame(frame);
	int packet_quantization_levels = (frame->quantization_levels < frames_in_buffer) ? (frame->quantization_levels) : (frames_in_buffer);
	
	char buffer[2048];
	PacketHeader *header = (PacketHeader *) buffer;
	char *buffer_ptr = (char *) (header + 1);
	int block_count = 0;
	int block_index = 0;
	int packet_count = 0;

	header->frame_no = htons(frame->frame_no);
	header->block_x = frame->width / 8;
	header->block_y = frame->height / 8;
	header->quantization_levels = packet_quantization_levels;
			
	for (int i = 0; i < frame->blocks; i++) {

		// RL encode channel values for different quantization levels into buffer
		for (int j = 0; j < packet_quantization_levels; j++) {

			buffer_ptr += rlEncodeBlock(
				outgoing_frame_buffer[j]->data_quantized[j].r + i*8*8,
				buffer_ptr);

			buffer_ptr += rlEncodeBlock(
				outgoing_frame_buffer[j]->data_quantized[j].g + i*8*8,
				buffer_ptr);

			buffer_ptr += rlEncodeBlock(
				outgoing_frame_buffer[j]->data_quantized[j].b + i*8*8,
				buffer_ptr);

		}
		
		block_count++;

		// send packet when having gathered a couple of blocks
		int packet_size = (int) (buffer_ptr - buffer);

		if ((packet_size > aggregate_size) || (i == frame->blocks - 1)) {
			
//			DBG_INFO(FMT("sending packet with %1% bytes in %2% blocks") % packet_size % block_count);
			
			header->block_index = htons(block_index);
			header->block_count = block_count;

			//payload->dbgPrintBuffer();

			// copy buffer
			boost::shared_ptr<CMessageBuffer> pkt(new CMessageBuffer(buffer_t(packet_size, (boctet_t*) buffer)));
			pkt->setFrom(this);
			pkt->setTo(next);
			pkt->setType(IMessage::t_outgoing);
			pkt->setProperty(IMessage::p_serviceId, frame->getProperty(IMessage::p_serviceId));
			pkt->setProperty(IMessage::p_destId, frame->getProperty(IMessage::p_destId));

			sendMessage(pkt);

			packet_count++;

			// reset loop variables
			buffer_ptr = (char *) (header + 1);
			block_index = i + 1;
			block_count = 0;
			
		}
		
	}
	
//	DBG_INFO(FMT("Sent %1% blocks in %2% packets") % frame->blocks % packet_count);

	return 0;

}


/**
 * @brief Deserialize packet contents into quantized DCT coefficients
 *
 * @param frame	Pointer to video frame
 */
void IVid_Serializer::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {
//	DBG_INFO(FMT("entering IN::[%1%]") % className);

	boost::shared_ptr<CMessageBuffer> packet = msg->cast<CMessageBuffer>();

	if (packet.get() == NULL) {
		DBG_ERROR("Received type is not a valid packet!");
		throw EUnhandledMessage();

	}
	
	//packet->getSerialBuffer()->dbgPrintBuffer();
	message_t payload = packet->pop_buffer();
	assert(payload.length() == 1);
	const PacketHeader *header = (const PacketHeader *) (payload.at(0).data());
	
	//	DBG_INFO(FMT("CVid:  %1% for frame %2%") % ntohs(header->block_index) % ntohs(header->frame_no));

	boost::shared_ptr<CVideoFrame> current_frame = incoming_frame_buffer[0];
	
	if ((current_frame.get() == NULL) || (current_frame->frame_no != ntohs(header->frame_no)))
		current_frame = bufferNewIncomingFrame(payload, packet->getProperty<CStringValue>(IMessage::p_serviceId)->value());

	// RL decode packet contents to frame structure
	const char *buffer_ptr = (const char *) (header + 1);
	
	for (int i = 0; i < header->block_count; i++) {
		int I = ntohs(header->block_index) + i;
		
		for (int j = 0; j < header->quantization_levels; j++) {
		
			if ((incoming_frame_buffer[j] != NULL) && (incoming_frame_buffer[j]->block_quality[I] > j)) {

				// retrieve data only if received quality level is
				// lower (== better) to existent level. note that
				// for received frames, all quantization levels
				// are de-serialized into the same (single) quantization
				// buffer...

				if (incoming_frame_buffer[j]->block_quality[I] == 100) {
					// count a block only once, regardless of its FEC level
					incoming_frame_buffer[j]->statBlocksReceived++;
				}

				incoming_frame_buffer[j]->block_quality[I] = j;
				
				buffer_ptr += rlDecodeBlock(
					buffer_ptr,
					incoming_frame_buffer[j]->data_quantized[0].r + I*8*8);
					
				buffer_ptr += rlDecodeBlock(
					buffer_ptr,
					incoming_frame_buffer[j]->data_quantized[0].g + I*8*8);
					
				buffer_ptr += rlDecodeBlock(
					buffer_ptr,
					incoming_frame_buffer[j]->data_quantized[0].b + I*8*8);
				
			} else {
				// otherwise simply skip the data...
				// (could be sped up by storing length of RL encoded data)
				int crap[64];
				
				buffer_ptr += rlDecodeBlock(buffer_ptr, crap); // R
				buffer_ptr += rlDecodeBlock(buffer_ptr, crap); // G
				buffer_ptr += rlDecodeBlock(buffer_ptr, crap); // B
				
			}
		
		}
			
	}
}

boost::shared_ptr<CVideoFrame> IVid_Serializer::bufferNewIncomingFrame(message_t buf, const std::string& serviceId) {

	assert(buf.length() == 1);
	const PacketHeader *header = (const PacketHeader *) (buf.at(0).data());

	boost::shared_ptr<CVideoFrame> new_frame(new CVideoFrame(this, prev, header->block_x*8, header->block_y*8, ntohs(header->frame_no)));
	new_frame->setType(IMessage::t_incoming);
	new_frame->setProperty(IMessage::p_serviceId, new CStringValue(serviceId));

//	DBG_INFO(FMT("CVid: Creating new frame (seqno %1%)") % new_frame->frame_no);
	if (incoming_frame_buffer[0] != NULL && new_frame->frame_no < incoming_frame_buffer[0]->frame_no)
		DBG_WARNING(FMT("Incoming frame number %1% smaller than last one buffered (%2%)")
				% new_frame->frame_no % incoming_frame_buffer[0]->frame_no);

	new_frame->createBlockQualityBuffer();
	new_frame->createQuantizationBuffers(1);

	if ((incoming_frame_buffer[0] != NULL) && (incoming_frame_buffer[0]->width != new_frame->width)) {

		DBG_INFO(FMT("CVid:IN flushing buffer (res change %1% -> %2%)") % incoming_frame_buffer[0]->width % new_frame->width);

		for (int i = 1; i < buffer_size; i++) {

			if (incoming_frame_buffer[i].get() != NULL)
				incoming_frame_buffer[i].reset();

		}

		incoming_frame_buffer[0] = new_frame;

	} else {

		for (int i = 0; i < buffer_size; i++) {
			boost::shared_ptr<CVideoFrame> temp = incoming_frame_buffer[i];
			incoming_frame_buffer[i] = new_frame;

			new_frame = temp;

		}
		
		// frames removed from the buffer are sent towards the application
		// regardless how they may "look like"...
		if (new_frame != NULL) {
			boost::shared_ptr<IMessage> msg(new_frame);
			sendMessage(msg);

		}
		
	}
	
	return incoming_frame_buffer[0];

}

int IVid_Serializer::bufferOutgoingFrame(boost::shared_ptr<CVideoFrame> frame) {
	boost::shared_ptr<CVideoFrame> new_frame = frame;
	int last_frame = 0;
	
	if ((outgoing_frame_buffer[0].get() != NULL) && (outgoing_frame_buffer[0]->width != frame->width)) {
		DBG_INFO(FMT("CVid:OUT flushing buffer (res change %1% -> %2%)") % outgoing_frame_buffer[0]->width % new_frame->width);

		for (int i = 1; i < buffer_size; i++) {

			if (outgoing_frame_buffer[i].get() != NULL)
				outgoing_frame_buffer[i].reset();

		}

		outgoing_frame_buffer[0] = frame;

		return 1;

	}

	for (int i = 0; i < buffer_size; i++) {
		boost::shared_ptr<CVideoFrame> temp = outgoing_frame_buffer[i];
		outgoing_frame_buffer[i] = new_frame;
		
		if (new_frame.get() != NULL)
			last_frame = i;
		
		new_frame = temp;
		
	}
	
	return last_frame + 1;
	
}
			
int IVid_Serializer::rlEncodeBlock(int *blk, char *buffer) {
	short *first = (short *) buffer;
	char *index = (char *) (first + 1);
	
	int i = 1;
	
	*first = blk[0];
	
	blk[62] = 0; // fake ;-)
	blk[63] = 1; // fake ;-)
	
	while (i < 63) {
		char *oldIndex = (char *) index++;
		int count_nz = i;
		
		if (blk[i] != 0) {
			*index = blk[i++];
			index++;
			
			if (blk[i] != 0) {
				*index = blk[i++];
				index++;
				
				if (blk[i] != 0) {
					*index = blk[i++];
					index++;

				}
				
			}
		
		}
		
		int count_z = i;

		while (blk[i] == 0)
			i++;
		
		*oldIndex = ((i - count_z) << 2) | (count_z - count_nz);
		
	}
	
	return (int) (index - buffer);

}

int IVid_Serializer::rlDecodeBlock(const char *buffer, int *blk) {
	const short *first = (const short *) buffer;
	const char *index = (const char *) (first + 1);
	int i = 1;
	
	// memset(blk, 0, 8*8*4);

	blk[0] = *first;

	while (i < 63) {
		unsigned char counter = *((const unsigned char *) index++);
		unsigned char count_nz = counter & 3;
		
		while (count_nz-- > 0) {
			blk[i++] = *index;
			index++;

		}
		
		i += (counter >> 2);

	}
	
	return (int) (index - buffer);

}

int IVid_Serializer::rlSkipBlock(char *buffer) {
	short *first = (short *) buffer;
	char *index = (char *) (first + 1);
	int i = 1;
	
	while (i < 63) {
		unsigned char counter = *((unsigned char *) index++);
		unsigned char counter_nz = counter & 3;
		
		index += counter_nz;
		
		i += (counter >> 2);

	}
	
	return (int) (index - buffer);

}

/**
 * @brief	Set the number of FEC levels. This will flush currently buffered
 * 			frames.
 *
 * @param levels	Number of FEC levels. Allowed values are 0 (no FEC),
 * 					1 (one FEC level), 2 (two FEC levels)
 */
void IVid_Serializer::setFecLevels(int levels)
{
	assert(levels >= 0 && levels < (IVID_BUFFER_MAX_SIZE - 1)); // [0..2]

	buffer_size = levels + 1;

	for (int i = 1; i < IVID_BUFFER_MAX_SIZE; i++) {
		if (incoming_frame_buffer[i] != NULL)
			incoming_frame_buffer[i].reset();

		if (outgoing_frame_buffer[i] != NULL)
			outgoing_frame_buffer[i].reset();
	}


}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

const string ividSerializerNofecClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Serializer_NoFEC";
const string ividSerializerLofecClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Serializer_LoFEC";
const string ividSerializerHifecClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Serializer_HiFEC";
