/** @file
 * vid_idct.h
 *
 * @brief	Handles (inverse) discrete cosine transformations
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Sep 28, 2009
 *      Author: PCB
 *
 * Original sources from:
 *
 **************************************************************************
 *
 * libr263: fast H.263 encoder library
 *
 * Copyright (C) 1996, Roalt Aalmoes, Twente University
 * SPA multimedia group
 *
 * Based on Telenor TMN 1.6 encoder (Copyright (C) 1995, Telenor R&D)
 * created by Karl Lillevold 
 *
 * Author encoder: Roalt Aalmoes, 
 * 
 * Date: 31-07-96
 *
 **************************************************************************
 *
 * Some routines are translated from Gisle Bj�ntegaards's FORTRAN
 * routines by Robert.Danielsen@nta.no
 *
 **************************************************************************
 *
 * This source includes sources from Berkeley's MPEG-1 encoder
 * which are copyright of Berkeley University, California, USA
 *
 */

#ifndef VID_IDCT_H_
#define VID_IDCT_H_

#include "vid_structs.h"

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

extern const std::string ividIdctClassName;

/**
 * @brief	Building handling (inverse) discrete consine transformations
 */
class CVid_IDCT : public IVid_Component {

public:

	CVid_IDCT(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~CVid_IDCT();


	// from IVid_Component
	
	virtual int processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame);
	virtual int processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame);


protected:

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

};

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

#endif /* VID_IDCT_H_ */
