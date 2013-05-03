/** @file
 * vid_structs.h
 *
 * @brief Objects and structures for handling video blocks, chunks and frames.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Sep 23, 2009
 *      Author: PCB
 */

#ifndef VID_STRUCTS_H_
#define VID_STRUCTS_H_

#include "composableNetlet.h"

#include "messages.h"
#include "messageBuffer.h"

#include "nena.h"

#include <string>
#include <assert.h>

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

/**
 * @brief Video frame structure covering different working buffers.
 */
class CVideoFrame : public IMessage {

public:

	typedef struct {
		int *r;
		int *g;
		int *b;

	} ChannelData;
	
	int width;
	int height;
	int blocks;
	
	int statBlocksReceived;
	int statFecBlocksUsed;

	ChannelData data_bitmap;
	ChannelData data_coefficients;
	ChannelData *data_quantized;
	
	char *block_quality;
	
	int quantization_levels;
	
	int frame_no;

    /**
     * @brief Constructor for video frames in 8-bit RGBA 
     */
    CVideoFrame(
		IMessageProcessor *from,
		IMessageProcessor *to, 
		int width, 
		int height, 
		int frame_no);

	/**
	 * @brief Destructor.
	 */
	virtual ~CVideoFrame();

	void createBlockQualityBuffer();
	void createQuantizationBuffers(int levels);
	
protected:
	
	ChannelData createWorkingBuffer();
	void releaseWorkingBuffer(ChannelData cd);

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

/**
 * @brief	Generic video netlet component 
 */
class IVid_Component : public IBuildingBlock {
public:

	IVid_Component(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, std::string component_id);
	virtual ~IVid_Component();
	

	// new virtuals
	
	virtual int processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame);
	virtual int processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame);

	/**
	 * @brief	Set the number of FEC levels. By default, this does nothing.
	 *
	 * @param levels	Number of FEC levels. Allowed values are 0 (no FEC),
	 * 					1 (one FEC level), 2 (two FEC levels)
	 */
	virtual void setFecLevels(int levels);


	// from IMessageProcessor

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

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

#endif /* VID_STRUCTS_H_ */
