/** @file
 * vid_codec.h
 *
 * @brief	Video CoDec
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Oct 6 , 2009
 *      Author: PCB
 */

#ifndef VID_CODEC_H_
#define VID_CODEC_H_

#include "vid_structs.h"

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividCodecClassName;

/**
 * @brief	Building block combining all other building block into one :-)
 */
class CVid_Codec : public IVid_Component {

public:

	CVid_Codec(CNena *nodeArch, IMessageScheduler *sched, INetlet *netlet, const std::string id);
	virtual ~CVid_Codec();
	

	// from IVid_Component
	
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);


protected:


	// from vid_convert.h /////////////////////////////////////////////////
	
	typedef struct {
		unsigned int width;
		unsigned int height;
		
	} __attribute__((packed)) FrameHeader;
	
	int outgoing_frame_no;
	
	void convert_in(CVideoFrame *frame);
	CVideoFrame *convert_out(IMessage *msg) throw (EUnhandledMessage);


	// from vid_idct.h ////////////////////////////////////////////////////
	
	static int zigzag[8][8];
	
	static const int W1;
	static const int W2;
	static const int W3;
	static const int W5;
	static const int W6;
	static const int W7;
		
	int iclip[1024]; /* clipping table */
	int *iclp;
	
	void dct(int *block, int *coeff);

	void idct(int *coeff, int *block);
	void idctrow(int *blk);
	void idctcol(int *blk);

	void dct_in(CVideoFrame *frame);
	void dct_out(CVideoFrame *frame);


	// from vid_quantizer.h ///////////////////////////////////////////////

	static int DEQUANTIZER[64];
	static int QUANTIZER[64];
	
	int CLIP[512];
	
	virtual void dequantizeBlock(int *in, int *out, int level);
	virtual void quantizeBlock(int *in, int *no_fec, int *lo_fec, int *hi_fec);

	void quantize_in(CVideoFrame *frame);
	void quantize_out(CVideoFrame *frame);


	// from vid_serializer.h //////////////////////////////////////////////
	
	typedef struct {
		unsigned int frame_no;
		unsigned char block_x;
		unsigned char block_y;
		unsigned short block_index;
		unsigned char block_count;
		unsigned char quantization_levels;
		
	} __attribute__((packed)) PacketHeader;

	CVideoFrame *incoming_frame_buffer[4];
	CVideoFrame *outgoing_frame_buffer[4];

	CVideoFrame *bufferNewIncomingFrame(CMessageBuffer *packet);
	int bufferOutgoingFrame(CVideoFrame *frame);
	
	int rlEncodeBlock(int *blk, char *buffer);
	int rlDecodeBlock(char *buffer, int *blk);
	int rlSkipBlock(char *buffer);

	int buffer_size;
	int aggregate_size;
	
	CVideoFrame *frame_to_application;

	void serializer_in(IMessage *msg) throw (EUnhandledMessage);
	void serializer_out(CVideoFrame *frame);

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

#endif /* VID_CODEC_H_ */
