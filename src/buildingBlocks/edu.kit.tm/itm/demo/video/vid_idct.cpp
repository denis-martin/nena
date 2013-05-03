/** @file
 * vid_idct.cpp
 *
 * @brief	Handles (inverse) discrete cosine transformations
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 * Compile this one with '-O2 -funroll-loops' !
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

#include "vid_idct.h"

using namespace std;

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

int CVid_IDCT::zigzag[8][8] = {
	{  0,  1,  5,  6, 14, 15, 27, 28},
	{  2,  4,  7, 13, 16, 26, 29, 42},
	{  3,  8, 12, 17, 25, 30, 41, 43},
	{  9, 11, 18, 24, 31, 40, 44, 53},
	{ 10, 19, 23, 32, 39, 45, 52, 54},
	{ 20, 22, 33, 38, 46, 51, 55, 60},
	{ 21, 34, 37, 47, 50, 56, 59, 61},
	{ 35, 36, 48, 49, 57, 58, 62, 63}};

/*************************************************************************/
/*************************************************************************/

const int CVid_IDCT::W1 = 2841;	/* 2048*sqrt(2)*cos(1*pi/16) */
const int CVid_IDCT::W2 = 2676;	/* 2048*sqrt(2)*cos(2*pi/16) */
const int CVid_IDCT::W3 = 2408;	/* 2048*sqrt(2)*cos(3*pi/16) */
const int CVid_IDCT::W5 = 1609;	/* 2048*sqrt(2)*cos(5*pi/16) */
const int CVid_IDCT::W6 = 1108;	/* 2048*sqrt(2)*cos(6*pi/16) */
const int CVid_IDCT::W7 =  565;	/* 2048*sqrt(2)*cos(7*pi/16) */
		
/*************************************************************************/
/*************************************************************************/

const string ividIdctClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_IDCT";

CVid_IDCT::CVid_IDCT(
	CNena *nodeArch,
	IMessageScheduler *sched,
	IComposableNetlet *netlet,
	const string id

) : IVid_Component(nodeArch, sched, netlet, id) {

	className += "::CVid_IDCT";
	
	iclp = iclip + 512;
	
	for (int i = -512; i < 512; i++)
		iclp[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);

}

CVid_IDCT::~CVid_IDCT() {

}


/**
 * @brief Apply DCT to all 8x8 internal bitmap blocks, resulting in respective coefficients
 *
 * @param frame	Pointer to video frame
 */
int CVid_IDCT::processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame) {

	for (int i = 0; i < frame->blocks; i++) {
		int block_offset = i * 8 * 8;
		
		dct(frame->data_bitmap.r + block_offset, frame->data_coefficients.r + block_offset);
		dct(frame->data_bitmap.g + block_offset, frame->data_coefficients.g + block_offset);
		dct(frame->data_bitmap.b + block_offset, frame->data_coefficients.b + block_offset);
		
	}
	
	return 1;
	
}


/**
 * @brief Apply IDCT to all 8x8 internal coefficient blocks, resulting in respective bitmaps
 *
 * @param frame	Pointer to video frame
 */
int CVid_IDCT::processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame) {

	for (int i = 0; i < frame->blocks; i++) {
		int block_offset = i * 8 * 8;
		
		idct(frame->data_coefficients.r + block_offset, frame->data_bitmap.r + block_offset);
		idct(frame->data_coefficients.g + block_offset, frame->data_bitmap.g + block_offset);
		idct(frame->data_coefficients.b + block_offset, frame->data_bitmap.b + block_offset);
		
	}
	
	return 1;

}


void CVid_IDCT::dct(int *block, int *coeff) {
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


void CVid_IDCT::idct(int *coeff, int *block) {
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

void CVid_IDCT::idctrow(int *blk) {
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


void CVid_IDCT::idctcol(int *blk) {
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
