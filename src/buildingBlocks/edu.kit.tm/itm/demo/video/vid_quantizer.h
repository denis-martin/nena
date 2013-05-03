/** @file
 * vid_quantizer.h
 *
 * @brief	Handles the (de)quantization of DCT coefficients
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Sep 28, 2009
 *      Author: PCB
 */

#ifndef VID_QUANTIZER_H_
#define VID_QUANTIZER_H_

#include "vid_structs.h"

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividQuantizerClassName;

/**
 * @brief	Building block for (de)quantizing DCT coefficients
 */
class IVid_Quantizer : public IVid_Component {

protected:
	
	static int DEQUANTIZER[64];
	static int QUANTIZER[64];
	
	int CLIP[512];
	
	virtual void dequantizeBlock(int *in, int *out, int level);
	
public:

	IVid_Quantizer(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~IVid_Quantizer();


	// from IVid_Component
	
	virtual int processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame);

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividQuantizerNofecClassName;

class CVid_Quantizer_NoFEC : public IVid_Quantizer {

protected:

	virtual void quantizeBlock(int *in, int *no_fec);
	
public:

	CVid_Quantizer_NoFEC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id) :
		IVid_Quantizer(nodeArch, sched, netlet, id)
	{
	}

	virtual int processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame);

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividQuantizerLofecClassName;

class CVid_Quantizer_LoFEC : public IVid_Quantizer {

protected:

	virtual void quantizeBlock(int *in, int *no_fec, int *lo_fec);
	
public:

	CVid_Quantizer_LoFEC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id) :
		IVid_Quantizer(nodeArch, sched, netlet, id)
	{
	}
	
	virtual int processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame);

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividQuantizerHifecClassName;

class CVid_Quantizer_HiFEC : public IVid_Quantizer {

protected:

	int fec_levels;

	virtual void quantizeBlock(int *in, int *no_fec);
	virtual void quantizeBlock(int *in, int *no_fec, int *lo_fec);
	virtual void quantizeBlock(int *in, int *no_fec, int *lo_fec, int *hi_fec);
	
public:

	CVid_Quantizer_HiFEC(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id) :
		IVid_Quantizer(nodeArch, sched, netlet, id)
	{
		fec_levels = 2;
	}
	
	virtual int processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame);

	/**
	 * @brief	Set the number of FEC levels.
	 *
	 * @param levels	Number of FEC levels. Allowed values are 0 (no FEC),
	 * 					1 (one FEC level), 2 (two FEC levels)
	 */
	virtual void setFecLevels(int levels);

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

#endif /* VID_QUANTIZER_H_ */
