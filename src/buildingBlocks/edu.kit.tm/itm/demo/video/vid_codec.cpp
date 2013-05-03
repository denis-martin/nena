/** @file
 * vid_codec.cpp
 *
 * @brief	Video CoDec
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 * Compile this one with '-O2 -funroll-loops' !
 *
 *  Created on: Oct 6, 2009
 *      Author: PCB
 */

#include "vid_codec.h"

#include "debug.h"

using boost::shared_ptr;
using namespace std;

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

int CVid_Codec::zigzag[8][8] = {
	{  0,  1,  5,  6, 14, 15, 27, 28},
	{  2,  4,  7, 13, 16, 26, 29, 42},
	{  3,  8, 12, 17, 25, 30, 41, 43},
	{  9, 11, 18, 24, 31, 40, 44, 53},
	{ 10, 19, 23, 32, 39, 45, 52, 54},
	{ 20, 22, 33, 38, 46, 51, 55, 60},
	{ 21, 34, 37, 47, 50, 56, 59, 61},
	{ 35, 36, 48, 49, 57, 58, 62, 63}};

const int CVid_Codec::W1 = 2841;	/* 2048*sqrt(2)*cos(1*pi/16) */
const int CVid_Codec::W2 = 2676;	/* 2048*sqrt(2)*cos(2*pi/16) */
const int CVid_Codec::W3 = 2408;	/* 2048*sqrt(2)*cos(3*pi/16) */
const int CVid_Codec::W5 = 1609;	/* 2048*sqrt(2)*cos(5*pi/16) */
const int CVid_Codec::W6 = 1108;	/* 2048*sqrt(2)*cos(6*pi/16) */
const int CVid_Codec::W7 =  565;	/* 2048*sqrt(2)*cos(7*pi/16) */
		
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

const string ividCodecClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_CoDec";

CVid_Codec::CVid_Codec(
	CNena *nodeArch,
	IMessageScheduler *sched,
	INetlet *netlet,
	const string id

) : IVid_Component(nodeArch, sched, id) {

	className += "::CVid_Codec";

	// converter //////////////////////////////////////////////////////////
	
	outgoing_frame_no = 0;

	
	// idct ///////////////////////////////////////////////////////////////
	
	iclp = iclip + 512;
	
	for (int i = -512; i < 512; i++)
		iclp[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
	
	
	// quantizer //////////////////////////////////////////////////////////
	
	for (int i = 0; i < 512; i++) {
		int j = i - 256;
		
		if (j < -128)
			j = -128;
	
		if (j > 127)
			j = 127;
		
		CLIP[i] = j;
		
	}
	
	
	// serializer /////////////////////////////////////////////////////////

	buffer_size = fec_levels + 1;
	assert(buffer_size < 4);
	
	aggregate_size = 1024;
	
	for (int i = 0; i < buffer_size; i++) {
		incoming_frame_buffer[i] = NULL;
		outgoing_frame_buffer[i] = NULL;
		
	}
	
}

CVid_Codec::~CVid_Codec() {

}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

void CVid_Codec::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {
	CVideoFrame *frame = convert_out(msg);
	dct_out(frame);
	quantize_out(frame);
	serializer_out(frame);
	
}

void CVid_Codec::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {
	serializer_in(msg);
	
	if (frame_to_application != NULL) {
		quantize_in(frame_to_application);
		dct_in(frame_to_application);
		convert_in(frame_to_application);
		
	}
	
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

void CVid_Codec::convert_in(CVideoFrame *frame) {
	int buffer_size = frame->width*frame->height*3 + sizeof(FrameHeader);
	CBuffer *buffer = new CBuffer(buffer_size);
	buffer->resize(buffer_size);
	FrameHeader *header = (FrameHeader *) (&(buffer->at(0)));
	unsigned char *rgba = (unsigned char *) (header + 1);

	// convert the 32-bit linear 8x8 block structure to an 8-bit RGBA bitmap
	// (ugly, but required by stupid GCC to generate _nice_ assembly...)

	header->width = frame->width;
	header->height = frame->height;

	int *R = frame->data_bitmap.r;
	int *G = frame->data_bitmap.g;
	int *B = frame->data_bitmap.b;

	for (int col = 0; col < frame->width / 8; col++) {
		unsigned char *RGBA = rgba + col*8*3;

		for (int row = 0; row < frame->height; row++) {

			for (int pixel = 0; pixel < 8; pixel++) {
				RGBA[pixel*3]     = *R++;
				RGBA[pixel*3 + 1] = *G++;
				RGBA[pixel*3 + 2] = *B++;

			}

			RGBA += frame->width*3;

		}

	}

	// build & send packet to application...
	DBG_INFO(FMT("Sending frame to application (resolution %1%x%2%, received %3% of %4% blocks, used %5% FEC blocks)") %
		frame->width % frame->height %
		frame->statBlocksReceived % frame->blocks %
		frame->statFecBlocksUsed);
	CMessageBuffer *pkt = new CMessageBuffer(this, prev, IMessage::t_incoming, buffer);

	pkt->setProperty(IMessage::p_serviceId, frame->getProperty(IMessage::p_serviceId));

	sendMessage(pkt);

	delete frame;

}

CVideoFrame *CVid_Codec::convert_out(IMessage *msg) throw (EUnhandledMessage) {

	boost::shared_ptr<CMessageBuffer> packet = msg->cast<CMessageBuffer>();

	if (packet == NULL) {
		DBG_ERROR("Received type is not a valid packet!");
		throw EUnhandledMessage();

	}

	shared_ptr<CMorphableValue> destId;
	if (!packet->hasProperty(IMessage::p_destId, destId))
		throw EUnhandledMessage("No destId specified, dropping frame");

	FrameHeader *header = (FrameHeader *) (&(packet->getPayload()->at(0)));

	// convert 8-bit RGBA data into a 32-bit linear 8x8 block structure
	// (ugly, but required to let stupid GCC generate _nice_ assembly...)

	CVideoFrame *frame = new CVideoFrame(this, next, header->width, header->height, outgoing_frame_no);
	outgoing_frame_no++;

	unsigned char *rgba = (unsigned char *) (header + 1);

	int *R = frame->data_bitmap.r;
	int *G = frame->data_bitmap.g;
	int *B = frame->data_bitmap.b;

	for (int col = 0; col < frame->width / 8; col++) {
		unsigned char *RGBA = rgba + col*8*3;

		for (int row = 0; row < frame->height; row++) {

			for (int pixel = 0; pixel < 8; pixel++) {
				*R++ = RGBA[pixel*3];
				*G++ = RGBA[pixel*3 + 1];
				*B++ = RGBA[pixel*3 + 2];

			}

			RGBA += frame->width*3;

		}

	}

	frame->setProperty(packet->getProperty(IMessage::p_serviceId));
	frame->setProperty(IMessage::p_destId, destId);

	delete packet;
	
	return frame;

}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/**
 * @brief Apply DCT to all 8x8 internal bitmap blocks, resulting in respective coefficients
 *
 * @param frame	Pointer to video frame
 */
void CVid_Codec::dct_out(CVideoFrame *frame) {

	for (int i = 0; i < frame->blocks; i++) {
		int block_offset = i * 8 * 8;
		
		dct(frame->data_bitmap.r + block_offset, frame->data_coefficients.r + block_offset);
		dct(frame->data_bitmap.g + block_offset, frame->data_coefficients.g + block_offset);
		dct(frame->data_bitmap.b + block_offset, frame->data_coefficients.b + block_offset);
		
	}
	
}


/**
 * @brief Apply IDCT to all 8x8 internal coefficient blocks, resulting in respective bitmaps
 *
 * @param frame	Pointer to video frame
 */
void CVid_Codec::dct_in(CVideoFrame *frame) {

	for (int i = 0; i < frame->blocks; i++) {
		int block_offset = i * 8 * 8;
		
		idct(frame->data_coefficients.r + block_offset, frame->data_bitmap.r + block_offset);
		idct(frame->data_coefficients.g + block_offset, frame->data_bitmap.g + block_offset);
		idct(frame->data_coefficients.b + block_offset, frame->data_bitmap.b + block_offset);
		
	}

}


void CVid_Codec::dct(int *block, int *coeff) {
	float f0=.7071068, f1=.4903926, f2=.4619398, f3=.4157348;
	float f4=.3535534, f5=.2777851, f6=.1913417, f7=.0975452;
	float d[8][8];
	float b1[8];
	float b[8];

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < 8; j++) {
			b[j] = block[i*8 + j];
		
		}
		
		/* Horizontal transform */
		for (int j = 0; j < 4; j++) {
			int j1 = 7 - j;

			b1[j] = b[j] + b[j1];
			b1[j1] = b[j] - b[j1];
		
		}
	
		b[0] = b1[0] + b1[3];
		b[1] = b1[1] + b1[2];
		b[2] = b1[1] - b1[2];
		b[3] = b1[0] - b1[3];
		b[4] = b1[4];
		b[5] = (b1[6] - b1[5]) * f0;
		b[6] = (b1[6] + b1[5]) * f0;
		b[7] = b1[7];
		d[i][0] = (b[0] + b[1]) * f4;
		d[i][4] = (b[0] - b[1]) * f4;
		d[i][2] = b[2] * f6 + b[3] * f2;
		d[i][6] = b[3] * f6 - b[2] * f2;
		b1[4] = b[4] + b[5];
		b1[7] = b[7] + b[6];
		b1[5] = b[4] - b[5];
		b1[6] = b[7] - b[6];
		d[i][1] = b1[4] * f7 + b1[7] * f1;
		d[i][5] = b1[5] * f3 + b1[6] * f5;
		d[i][7] = b1[7] * f7 - b1[4] * f1;
		d[i][3] = b1[6] * f3 - b1[5] * f5;
		
	}
	
	
	/* Vertical transform */
	for (int i = 0; i < 8; i++) {
		
		for (int j = 0; j < 4; j++) {
			int j1 = 7 - j;
			
			b1[j] = d[j][i] + d[j1][i];
			b1[j1] = d[j][i] - d[j1][i];
			
		}
		
		b[0] = b1[0] + b1[3];
		b[1] = b1[1] + b1[2];
		b[2] = b1[1] - b1[2];
		b[3] = b1[0] - b1[3];
		b[4] = b1[4];
		b[5] = (b1[6] - b1[5]) * f0;
		b[6] = (b1[6] + b1[5]) * f0;
		b[7] = b1[7];
		d[0][i] = (b[0] + b[1]) * f4;
		d[4][i] = (b[0] - b[1]) * f4;
		d[2][i] = b[2] * f6 + b[3] * f2;
		d[6][i] = b[3] * f6 - b[2] * f2;
		b1[4] = b[4] + b[5];
		b1[7] = b[7] + b[6];
		b1[5] = b[4] - b[5];
		b1[6] = b[7] - b[6];
		d[1][i] = b1[4] * f7 + b1[7] * f1;
		d[5][i] = b1[5] * f3 + b1[6] * f5;
		d[7][i] = b1[7] * f7 - b1[4] * f1;
		d[3][i] = b1[6] * f3 - b1[5] * f5;

  }

  /* Zigzag - scanning */
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++)
      *(coeff + zigzag[i][j]) = (int)(d[i][j]);

}


void CVid_Codec::idct(int *coeff, int *block) {
	int *zigzag_ptr = &(zigzag[0][0]);
	int *block_ptr = block; 
	
	for (int i = 0; i < 8; i++) {
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));
		*(block_ptr++) = *(coeff + *(zigzag_ptr++));

	}
	
	for (int i = 0; i < 8; i++)
		idctrow(block + 8*i);
	
	for (int i = 0; i < 8; i++)
		idctcol(block + i);
	
}

void CVid_Codec::idctrow(int *blk) {
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;
	
	/* shortcut */
	if (!((x1 = blk[4]<<11) | (x2 = blk[6]) | (x3 = blk[2]) |
        (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3]))) {
	
		blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;

		return;
	
	}

	x0 = (blk[0] << 11) + 128; /* for proper rounding in the fourth stage */
	
	/* first stage */
	x8 = W7*(x4+x5);
	x4 = x8 + (W1-W7)*x4;
	x5 = x8 - (W1+W7)*x5;
	x8 = W3*(x6+x7);
	x6 = x8 - (W3-W5)*x6;
	x7 = x8 - (W3+W5)*x7;
	
	/* second stage */
	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6*(x3+x2);
	x2 = x1 - (W2+W6)*x2;
	x3 = x1 + (W2-W6)*x3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	
	/* third stage */
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181*(x4+x5)+128)>>8;
	x4 = (181*(x4-x5)+128)>>8;
	
	/* fourth stage */
	blk[0] = (x7+x1)>>8;
	blk[1] = (x3+x2)>>8;
	blk[2] = (x0+x4)>>8;
	blk[3] = (x8+x6)>>8;
	blk[4] = (x8-x6)>>8;
	blk[5] = (x0-x4)>>8;
	blk[6] = (x3-x2)>>8;
	blk[7] = (x7-x1)>>8;

}


void CVid_Codec::idctcol(int *blk) {
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;
	
	/* shortcut */
	if (!((x1 = (blk[32]<<8)) | (x2 = blk[48]) | (x3 = blk[16]) |
        (x4 = blk[8]) | (x5 = blk[56]) | (x6 = blk[40]) | (x7 = blk[24]))) {

	    blk[0] = blk[8] = blk[16] = blk[24] = blk[32] = blk[40] = blk[48] = blk[56] = iclp[(blk[0] + 32) >> 6];
		
		return;
	
	}
	
	x0 = (blk[8*0]<<8) + 8192;
	
	/* first stage */
	x8 = W7*(x4+x5) + 4;
	x4 = (x8+(W1-W7)*x4)>>3;
	x5 = (x8-(W1+W7)*x5)>>3;
	x8 = W3*(x6+x7) + 4;
	x6 = (x8-(W3-W5)*x6)>>3;
	x7 = (x8-(W3+W5)*x7)>>3;
	
	/* second stage */
	x8 = x0 + x1;
	x0 -= x1;
	x1 = W6*(x3+x2) + 4;
	x2 = (x1-(W2+W6)*x2)>>3;
	x3 = (x1+(W2-W6)*x3)>>3;
	x1 = x4 + x6;
	x4 -= x6;
	x6 = x5 + x7;
	x5 -= x7;
	
	/* third stage */
	x7 = x8 + x3;
	x8 -= x3;
	x3 = x0 + x2;
	x0 -= x2;
	x2 = (181*(x4+x5)+128)>>8;
	x4 = (181*(x4-x5)+128)>>8;
	
	/* fourth stage */
	blk[0] = iclp[(x7+x1)>>14];
	blk[8] = iclp[(x3+x2)>>14];
	blk[16] = iclp[(x0+x4)>>14];
	blk[24] = iclp[(x8+x6)>>14];
	blk[32] = iclp[(x8-x6)>>14];
	blk[40] = iclp[(x0-x4)>>14];
	blk[48] = iclp[(x3-x2)>>14];
	blk[56] = iclp[(x7-x1)>>14];
	
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/**
 * @brief Dequantize DCT coefficients from given frame
 *
 * @param frame	Pointer to video frame
 */
void CVid_Codec::quantize_in(CVideoFrame *frame) {

	for (int i = 0; i < frame->blocks; i++) {

		if (frame->block_quality[i] < 3) {
			int block_offset = i * 8 * 8;
			int block_quality = (1 << (2 * frame->block_quality[i]));

			if (block_quality > 1)
				frame->statFecBlocksUsed++;

			dequantizeBlock(
				frame->data_quantized[0].r + block_offset,
				frame->data_coefficients.r + block_offset,
				block_quality);

			dequantizeBlock(
				frame->data_quantized[0].g + block_offset,
				frame->data_coefficients.g + block_offset,
				block_quality);

			dequantizeBlock(
				frame->data_quantized[0].b + block_offset,
				frame->data_coefficients.b + block_offset,
				block_quality);
			
		}
		
	}
	
}


/**
 * @brief Quantize DCT coefficients from given frame
 *
 * @param frame	Pointer to video frame
 */
void CVid_Codec::quantize_out(CVideoFrame *frame) {
	
	frame->createQuantizationBuffers(1 + 2);

	for (int i = 0; i < frame->blocks; i++) {
		int block_offset = i * 8 * 8;
		
		quantizeBlock(
			frame->data_coefficients.r + block_offset,
			frame->data_quantized[0].r + block_offset,
			frame->data_quantized[1].r + block_offset,
			frame->data_quantized[2].r + block_offset);
		
		quantizeBlock(
			frame->data_coefficients.g + block_offset,
			frame->data_quantized[0].g + block_offset,
			frame->data_quantized[1].g + block_offset,
			frame->data_quantized[2].g + block_offset);
		
		quantizeBlock(
			frame->data_coefficients.b + block_offset,
			frame->data_quantized[0].b + block_offset,
			frame->data_quantized[1].b + block_offset,
			frame->data_quantized[2].b + block_offset);
		
	}
	
}

void CVid_Codec::dequantizeBlock(int *in, int *out, int level) {

	for (int i = 0; i < 64; i++)
		out[i] = in[i] * (DEQUANTIZER[i] / 2 * level);

}

void CVid_Codec::quantizeBlock(int *in, int *no_fec, int *lo_fec, int *hi_fec) {

	// first value should stay uncliped
	int v = in[0] * QUANTIZER[0] / 32768;
		
	no_fec[0] = v;
	lo_fec[0] = v / 4;
	hi_fec[0] = v / 16;
	
	// clip remaining values to [-128, ..., 127]
	for (int i = 1; i < 64; i++) {
		int v1 = CLIP[256 + in[i] * QUANTIZER[i] / 32768];
		no_fec[i] = v1;
		
		int v2 = v1 / 4;
		lo_fec[i] = v2;
		
		int v3 = v2 / 4;
		hi_fec[i] = v3;

	}

}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/**
 * @brief Serialize an outgoing frame 
 *
 * @param frame	Pointer to video frame
 */
void CVid_Codec::serializer_out(CVideoFrame *frame) {
	
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

	header->frame_no = htonl(frame->frame_no);
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

			CBuffer *payload = new CBuffer((unsigned char *) buffer, packet_size);
			//payload->dbgPrintBuffer();
			CMessageBuffer *pkt = new CMessageBuffer(this, next, IMessage::t_outgoing, payload);

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
	
	DBG_INFO(FMT("Sent %1% blocks in %2% packets") % frame->blocks % packet_count);

}


/**
 * @brief Deserialize packet contents into quantized DCT coefficients
 *
 * @param frame	Pointer to video frame
 */
void CVid_Codec::serializer_in(IMessage *msg) throw (EUnhandledMessage) {
//	DBG_INFO(FMT("entering IN::[%1%]") % className);

	boost::shared_ptr<CMessageBuffer> packet = msg->cast<CMessageBuffer>();

	if (packet.get() == NULL) {
		DBG_ERROR("Received type is not a valid packet!");
		throw EUnhandledMessage();

	}
	
	frame_to_application = NULL;
	
	//packet->getSerialBuffer()->dbgPrintBuffer();

	PacketHeader *header = (PacketHeader *) (packet->getSerialBuffer()->getRemainingBuffer());
	
	//	DBG_INFO(FMT("CVid: recvd block %1% for frame %2%") % ntohs(header->block_index) % ntohl(header->frame_no));

	CVideoFrame *current_frame = incoming_frame_buffer[0];
	
	if ((current_frame == NULL) || (current_frame->frame_no != ntohl(header->frame_no)))
		current_frame = bufferNewIncomingFrame(packet);

	// RL decode packet contents to frame structure
	char *buffer_ptr = (char *) (header + 1);
	
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
	
	delete packet;
	
}

CVideoFrame *CVid_Codec::bufferNewIncomingFrame(CMessageBuffer *packet) {
	PacketHeader *header = (PacketHeader *) (packet->getSerialBuffer()->getRemainingBuffer());

	CVideoFrame *new_frame = new CVideoFrame(this, prev, header->block_x*8, header->block_y*8, ntohl(header->frame_no));
	new_frame->setType(IMessage::t_incoming);
	new_frame->setProperty(IMessage::p_serviceId, packet->getProperty(IMessage::p_serviceId));

	DBG_INFO(FMT("CVid: Creating new frame (seqno %1%)") % new_frame->frame_no);

	new_frame->createBlockQualityBuffer();
	new_frame->createQuantizationBuffers(1);

	if ((incoming_frame_buffer[0] != NULL) && (incoming_frame_buffer[0]->width != new_frame->width)) {

		DBG_INFO(FMT("CVid:IN flushing buffer (res change %1% -> %2%)") % incoming_frame_buffer[0]->width % new_frame->width);

		for (int i = 1; i < buffer_size; i++) {

			if (incoming_frame_buffer[i] != NULL)
				delete incoming_frame_buffer[i];

			incoming_frame_buffer[i] = NULL;

		}

		incoming_frame_buffer[0] = new_frame;

	} else {

		for (int i = 0; i < buffer_size; i++) {
			CVideoFrame *temp = incoming_frame_buffer[i];
			incoming_frame_buffer[i] = new_frame;

			new_frame = temp;

		}
		
		// frames removed from the buffer are sent towards the application
		// regardless how they may "look like"...
		if (new_frame != NULL)
			frame_to_application = new_frame;
		
	}
	
	return incoming_frame_buffer[0];

}

int CVid_Codec::bufferOutgoingFrame(CVideoFrame *frame) {
	CVideoFrame *new_frame = frame;
	int last_frame = 0;
	
	if ((outgoing_frame_buffer[0] != NULL) && (outgoing_frame_buffer[0]->width != frame->width)) {
		DBG_INFO(FMT("CVid:OUT flushing buffer (res change %1% -> %2%)") % outgoing_frame_buffer[0]->width % new_frame->width);

		for (int i = 1; i < buffer_size; i++) {

			if (outgoing_frame_buffer[i] != NULL)
				delete outgoing_frame_buffer[i];

			outgoing_frame_buffer[i] = NULL;

		}

		outgoing_frame_buffer[0] = frame;

		return 1;

	}

	for (int i = 0; i < buffer_size; i++) {
		CVideoFrame *temp = outgoing_frame_buffer[i];
		outgoing_frame_buffer[i] = new_frame;
		
		if (new_frame != NULL)
			last_frame = i;
		
		new_frame = temp;
		
	}
	
	if (new_frame != NULL)
		delete new_frame;
	
	return last_frame + 1;
	
}
			
int CVid_Codec::rlEncodeBlock(int *blk, char *buffer) {
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

int CVid_Codec::rlDecodeBlock(char *buffer, int *blk) {
	short *first = (short *) buffer;
	char *index = (char *) (first + 1);
	int i = 1;
	
	// memset(blk, 0, 8*8*4);

	blk[0] = *first;

	while (i < 63) {
		unsigned char counter = *((unsigned char *) index++);
		unsigned char count_nz = counter & 3;
		
		while (count_nz-- > 0) {
			blk[i++] = *index;
			index++;

		}
		
		i += (counter >> 2);

	}
	
	return (int) (index - buffer);

}

int CVid_Codec::rlSkipBlock(char *buffer) {
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


/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
