/** @file
 * vid_converter.cpp
 *
 * @brief	Handles the conversion between application bitmaps and the netlet's internal (working) representation.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 * Compile this one with '-O2 -funroll-loops' !
 *
 *  Created on: Sep 28, 2009
 *      Author: PCB
 */

#include "vid_converter.h"

#include "messageBuffer.h"

using boost::shared_ptr;
using namespace std;

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

const string ividConverterClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Converter";

CVid_Converter::CVid_Converter(
	CNena *nodeArch,
	IMessageScheduler *sched,
	IComposableNetlet *netlet,
	const string id

) : IVid_Component(nodeArch, sched, netlet, id) {

	className += "::CVid_Converter";

	outgoing_frame_no = 0;

}

CVid_Converter::~CVid_Converter() {

}


/**
 * @brief Convert an outgoing frame from 8-bit RGBA to 32-bit signed channel values
 *
 * @param frame	Pointer to video frame
 */
void CVid_Converter::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {

	boost::shared_ptr<CMessageBuffer> packet = msg->cast<CMessageBuffer>();

	if (packet.get() == NULL) {
		DBG_ERROR("Received type is not a valid packet!");
		throw EUnhandledMessage();

	}

	boost::shared_ptr<CMorphableValue> destId;
	if (!packet->hasProperty(IMessage::p_destId, destId))
		throw EUnhandledMessage("No destId specified, dropping frame");

	assert(packet->getBuffer().length() == 1);
	const FrameHeader *header = (const FrameHeader *) packet->getBuffer().at(0).data();

	// convert 8-bit RGBA data into a 32-bit linear 8x8 block structure
	// (ugly, but required to let stupid GCC generate _nice_ assembly...)

	boost::shared_ptr<CVideoFrame> frame(new CVideoFrame(this, next, header->width, header->height, outgoing_frame_no));
	outgoing_frame_no++;

	const unsigned char *rgba = (const unsigned char *) (header + 1);

	int *R = frame->data_bitmap.r;
	int *G = frame->data_bitmap.g;
	int *B = frame->data_bitmap.b;

/*	FILE *f = fopen("tmp.ppm", "wb");
	fprintf(f, "P6\n# bla\n%i %i\n255\n", header->width, header->height);
	fwrite(rgba, 1, header->width*header->height*3, f);
	fclose(f);*/

	for (int col = 0; col < frame->width / 8; col++) {
		const unsigned char *RGBA = rgba + col*8*3;

		for (int row = 0; row < frame->height; row++) {

			for (int pixel = 0; pixel < 8; pixel++) {
				*R++ = RGBA[pixel*3];
				*G++ = RGBA[pixel*3 + 1];
				*B++ = RGBA[pixel*3 + 2];

			}

			RGBA += frame->width*3;

		}

	}

	frame->setProperty(IMessage::p_serviceId, packet->getProperty(IMessage::p_serviceId));
	frame->setProperty(IMessage::p_destId, destId);

	sendMessage(frame);
}


/**
 * @brief Convert a frame from the internal representation to an 8-bit RGBA bitmap
 *
 * @param frame	Pointer to video frame
 */
int CVid_Converter::processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame) {

	int buffer_size = frame->width*frame->height*3 + sizeof(FrameHeader);
	shared_ptr<CMessageBuffer> buffer(new CMessageBuffer(buffer_size));
	FrameHeader *header = (FrameHeader *) (buffer->getBuffer().at(0).mutable_data());
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

/*	char filename[64];
	sprintf(filename, "frame.%i.ppm", frame->frame_no);

	FILE *f = fopen(filename, "wb");
	fprintf(f, "P6\n# bla\n%i %i\n255\n", frame->width, frame->height);
	fwrite(rgba, 1, frame->width*frame->height*3, f);
	fclose(f);*/

	// build & send packet to application...
//	DBG_INFO(FMT("Sending frame to application (resolution %1%x%2%, received %3% of %4% blocks, used %5% FEC blocks)") %
//		frame->width % frame->height %
//		frame->statBlocksReceived % frame->blocks %
//		frame->statFecBlocksUsed);
	if (frame->statBlocksReceived < frame->blocks)
		DBG_INFO(FMT("Sending incomplete frame to application (resolution %1%x%2%, received %3% of %4% blocks, used %5% FEC blocks)") %
			frame->width % frame->height %
			frame->statBlocksReceived % frame->blocks %
			frame->statFecBlocksUsed);

	buffer->setFrom(this);
	buffer->setTo(prev);
	buffer->setType(IMessage::t_incoming);
	buffer->setProperty(IMessage::p_serviceId, frame->getProperty(IMessage::p_serviceId));

	sendMessage(buffer);

	return 0;

}


/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
