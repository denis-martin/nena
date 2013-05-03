/** @file
 * vid_structs.cpp
 *
 * @brief Objects and structures for handling video blocks, chunks and frames.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Sep 25, 2009 (my birthday!)
 *      Author: PCB
 */

#include "vid_structs.h"

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

/**
 * @brief Constructor for video frames from 8-bit RGBA 
 */
CVideoFrame::CVideoFrame(
	IMessageProcessor *from,
	IMessageProcessor *to, 
	int width, 
	int height, 
	int frame_no

) : IMessage(from, to, t_outgoing) {

	assert((width % 8) == 0);
	assert((height % 8) == 0);
	this->width = width;
	this->height = height;
	blocks = (width*height) / (8*8);
	statBlocksReceived = 0;
	statFecBlocksUsed = 0;
	data_quantized = NULL;
	quantization_levels = 0;
	
	this->frame_no = frame_no;
	
	block_quality = NULL;
	
	// initialize working buffers - takes lots of RAM :-(
	
	data_bitmap = createWorkingBuffer();
	data_coefficients = createWorkingBuffer();

}

/**
 * @brief Destructor.
 */
CVideoFrame::~CVideoFrame() {

	for (int i = 0; i < quantization_levels; i++)
		releaseWorkingBuffer(data_quantized[i]);
	
	delete [] data_quantized;
	data_quantized = NULL;
	
	releaseWorkingBuffer(data_coefficients);
	releaseWorkingBuffer(data_bitmap);
	
	if (block_quality != NULL) {
		delete [] block_quality;
		block_quality = NULL;
	}

}

void CVideoFrame::createBlockQualityBuffer() {
	
	block_quality = new char[blocks];
	
	memset(block_quality, 100, blocks);
	
}

void CVideoFrame::createQuantizationBuffers(int levels) {
	quantization_levels = levels;
	
	data_quantized = new ChannelData[quantization_levels];
	
	for (int i = 0; i < quantization_levels; i++)
		data_quantized[i] = createWorkingBuffer();
	
}

CVideoFrame::ChannelData CVideoFrame::createWorkingBuffer() {
	ChannelData cd;
	
	cd.r = new int[width*height];
	cd.g = new int[width*height];
	cd.b = new int[width*height];
	
	memset(cd.r, 0, width*height*sizeof(int));
	memset(cd.g, 0, width*height*sizeof(int));
	memset(cd.b, 0, width*height*sizeof(int));

	return cd;
	
}

void CVideoFrame::releaseWorkingBuffer(ChannelData cd) {
	delete [] cd.b;
	delete [] cd.g;
	delete [] cd.r;
	
	cd.b = cd.g = cd.r = NULL;
	
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

IVid_Component::IVid_Component(
	CNena *nodeArch,
	IMessageScheduler *sched,
	IComposableNetlet *netlet,
	std::string id

) : IBuildingBlock(nodeArch, sched, netlet, id)
{
	className += "::IVid_Component";
}

IVid_Component::~IVid_Component() {

}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void IVid_Component::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
	
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void IVid_Component::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();

}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void IVid_Component::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {
	
//	DBG_INFO(FMT("entering OUT::[%1%]") % className);

	// dynamic cast only used to detect bugs early

	boost::shared_ptr<CVideoFrame> frame = msg->cast<CVideoFrame>();

	if (frame.get() == NULL) {
		DBG_ERROR("Received type is not a valid video frame!");
		throw EUnhandledMessage();
		
	}

	if (this->processOutgoingFrame(frame)) {
		frame->setFrom(this);
		frame->setTo(next);
		
		sendMessage(frame);
		
	} /* else {
		DBG_INFO(FMT("OUT::[%1%]: no further processing desired") % className);

	} */

//	DBG_INFO(FMT("leaving OUT::[%1%]") % className);
	
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void IVid_Component::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {

//	DBG_INFO(FMT("entering IN::[%1%]") % className);

	// dynamic cast only used to detect bugs early

	boost::shared_ptr<CVideoFrame> frame = msg->cast<CVideoFrame>();

	if (frame.get() == NULL) {
		DBG_ERROR("Received type is not a valid video frame!");
		throw EUnhandledMessage();

	}
	
	if (this->processIncomingFrame(frame)) {
		frame->setFrom(this);
		frame->setTo(prev);
		
		sendMessage(frame);
		
	}

//	DBG_INFO(FMT("leaving IN::[%1%]") % className);

}

int IVid_Component::processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame) {
	DBG_DEBUG(FMT("%1%: processOutgoingFrame() NYI") % getClassName());
	return 0;
	
}

int IVid_Component::processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame) {
	DBG_DEBUG(FMT("%1%: processIncomingFrame() NYI") % getClassName());
	return 0;

}

/**
 * @brief	Set the number of FEC levels. By default, this does nothing.
 *
 * @param levels	Number of FEC levels. Allowed values are 0 (no FEC),
 * 					1 (one FEC level), 2 (two FEC levels)
 */
void IVid_Component::setFecLevels(int levels)
{
	// nothing
}

