/** @file
 * vid_converter.h
 *
 * @brief	Handles the conversion between application bitmaps and the netlet's internal (working) representation.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Sep 28, 2009
 *      Author: PCB
 */

#ifndef VID_CONVERTER_H_
#define VID_CONVERTER_H_

#include "vid_structs.h"

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividConverterClassName;

/**
 * @brief	Building block to convert bitmaps into an internal representation and vice versa
 */
class CVid_Converter : public IVid_Component {

protected:

	typedef struct {
		uint32_t width;
		uint32_t height;
		
	} __attribute__((packed)) FrameHeader;
	
	int outgoing_frame_no;

public:

	CVid_Converter(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~CVid_Converter();

	
	// from IBuildingBlock
	
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);


	// from IVid_Component
	
	virtual int processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame);

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

#endif /* VID_CONVERTER_H_ */
