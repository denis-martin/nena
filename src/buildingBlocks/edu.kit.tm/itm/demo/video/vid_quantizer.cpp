/** @file
 * vid_quantizer.cpp
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

#include "vid_quantizer.h"

using namespace std;

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

int IVid_Quantizer::QUANTIZER[64] = {
	65536/16,  65536/11,  65536/10,  65536/16,  65536/24,  65536/40,  65536/51,  65536/61,
	65536/12,  65536/12,  65536/14,  65536/19,  65536/26,  65536/58,  65536/60,  65536/55,
	65536/14,  65536/13,  65536/16,  65536/24,  65536/40,  65536/57,  65536/69,  65536/56,
	65536/14,  65536/17,  65536/22,  65536/29,  65536/51,  65536/87,  65536/80,  65536/62,
	65536/18,  65536/22,  65536/37,  65536/58,  65536/68, 65536/109, 65536/103,  65536/77,
	65536/24,  65536/35,  65536/55,  65536/64,  65536/81, 65536/104, 65536/113,  65536/92,
	65536/49,  65536/64,  65536/78,  65536/87, 65536/103, 65536/121, 65536/120,  65536/101,
	65536/72,  65536/92,  65536/95,  65536/98, 65536/112, 65536/100, 65536/103,  65536/99};
	
int IVid_Quantizer::DEQUANTIZER[64] = {
	16,  11,  10,  16,  24,  40,  51,  61,
	12,  12,  14,  19,  26,  58,  60,  55,
	14,  13,  16,  24,  40,  57,  69,  56,
	14,  17,  22,  29,  51,  87,  80,  62,
	18,  22,  37,  58,  68, 109, 103,  77,
	24,  35,  55,  64,  81, 104, 113,  92,
	49,  64,  78,  87, 103, 121, 120, 101,
	72,  92,  95,  98, 112, 100, 103,  99};

/*************************************************************************/
/*************************************************************************/

const string ividQuantizerClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Quantizer";

IVid_Quantizer::IVid_Quantizer(
	CNena *nodeArch,
	IMessageScheduler *sched,
	IComposableNetlet *netlet,
	const string id

) : IVid_Component(nodeArch, sched, netlet, id) {

	className += "::CVid_Quantizer";
	
	for (int i = 0; i < 512; i++) {
		int j = i - 256;
		
		if (j < -128)
			j = -128;
	
		if (j > 127)
			j = 127;
		
		CLIP[i] = j;
		
	}
	
}

IVid_Quantizer::~IVid_Quantizer() {

}


/**
 * @brief Dequantize DCT coefficients from given frame
 *
 * @param frame	Pointer to video frame
 */
int IVid_Quantizer::processIncomingFrame(boost::shared_ptr<CVideoFrame>& frame) {

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
	
	return 1;

}

void IVid_Quantizer::dequantizeBlock(int *in, int *out, int level) {

	for (int i = 0; i < 64; i++)
		out[i] = in[i] * (DEQUANTIZER[i] / 2 * level);

}


/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

const string ividQuantizerNofecClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Quantizer_NoFEC";

/**
 * @brief Quantize DCT coefficients from given frame
 *
 * @param frame	Pointer to video frame
 */
int CVid_Quantizer_NoFEC::processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame) {
	
	frame->createQuantizationBuffers(1);

	for (int i = 0; i < frame->blocks; i++) {
		int block_offset = i * 8 * 8;
		
		quantizeBlock(
			frame->data_coefficients.r + block_offset,
			frame->data_quantized[0].r + block_offset);
		
		quantizeBlock(
			frame->data_coefficients.g + block_offset,
			frame->data_quantized[0].g + block_offset);
		
		quantizeBlock(
			frame->data_coefficients.b + block_offset,
			frame->data_quantized[0].b + block_offset);
		
	}
	
	return 1;

}

void CVid_Quantizer_NoFEC::quantizeBlock(int *in, int *no_fec) {

	// first value should stay uncliped
	int v = in[0] * QUANTIZER[0] / 32768;
		
	no_fec[0] = v;
	
	// clip remaining values to [-128, ..., 127]
	for (int i = 1; i < 64; i++) {
		int v1 = CLIP[256 + in[i] * QUANTIZER[i] / 32768];
		no_fec[i] = v1;
		
	}

}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

const string ividQuantizerLofecClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Quantizer_LoFEC";

/**
 * @brief Quantize DCT coefficients from given frame
 *
 * @param frame	Pointer to video frame
 */
int CVid_Quantizer_LoFEC::processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame) {
	
	frame->createQuantizationBuffers(1 + 1);

	for (int i = 0; i < frame->blocks; i++) {
		int block_offset = i * 8 * 8;
		
		quantizeBlock(
			frame->data_coefficients.r + block_offset,
			frame->data_quantized[0].r + block_offset,
			frame->data_quantized[1].r + block_offset);
		
		quantizeBlock(
			frame->data_coefficients.g + block_offset,
			frame->data_quantized[0].g + block_offset,
			frame->data_quantized[1].g + block_offset);
		
		quantizeBlock(
			frame->data_coefficients.b + block_offset,
			frame->data_quantized[0].b + block_offset,
			frame->data_quantized[1].b + block_offset);
		
	}
	
	return 1;

}

void CVid_Quantizer_LoFEC::quantizeBlock(int *in, int *no_fec, int *lo_fec) {

	// first value should stay uncliped
	int v = in[0] * QUANTIZER[0] / 32768;
		
	no_fec[0] = v;
	lo_fec[0] = v / 4;
	
	// clip remaining values to [-128, ..., 127]
	for (int i = 1; i < 64; i++) {
		int v1 = CLIP[256 + in[i] * QUANTIZER[i] / 32768];
		no_fec[i] = v1;
		
		int v2 = v1 / 4;
		lo_fec[i] = v2;

	}

}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

const string ividQuantizerHifecClassName = "bb://edu.kit.tm/itm/demo/video/BB_Vid_Quantizer_HiFEC";

/**
 * @brief Quantize DCT coefficients from given frame
 *
 * @param frame	Pointer to video frame
 */
int CVid_Quantizer_HiFEC::processOutgoingFrame(boost::shared_ptr<CVideoFrame>& frame) {
	
	switch (fec_levels) {

	case 0: { // no FEC

		frame->createQuantizationBuffers(1);

		for (int i = 0; i < frame->blocks; i++) {
			int block_offset = i * 8 * 8;

			quantizeBlock(
				frame->data_coefficients.r + block_offset,
				frame->data_quantized[0].r + block_offset);

			quantizeBlock(
				frame->data_coefficients.g + block_offset,
				frame->data_quantized[0].g + block_offset);

			quantizeBlock(
				frame->data_coefficients.b + block_offset,
				frame->data_quantized[0].b + block_offset);

		}

		break;
	}

	case 1: { // lo FEC
		
		frame->createQuantizationBuffers(1 + 1);

		for (int i = 0; i < frame->blocks; i++) {
			int block_offset = i * 8 * 8;

			quantizeBlock(
				frame->data_coefficients.r + block_offset,
				frame->data_quantized[0].r + block_offset,
				frame->data_quantized[1].r + block_offset);

			quantizeBlock(
				frame->data_coefficients.g + block_offset,
				frame->data_quantized[0].g + block_offset,
				frame->data_quantized[1].g + block_offset);

			quantizeBlock(
				frame->data_coefficients.b + block_offset,
				frame->data_quantized[0].b + block_offset,
				frame->data_quantized[1].b + block_offset);

		}
		
		break;
	}

	case 2: { // hi FEC
		
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
		
		break;
	}
	
	default: {
		assert(false);
	}

	} // switch

	return 1;

}

/**
 * @brief	Quantize block (w/o FEC)
 */
void CVid_Quantizer_HiFEC::quantizeBlock(int *in, int *no_fec) {

	// first value should stay uncliped
	int v = in[0] * QUANTIZER[0] / 32768;

	no_fec[0] = v;

	// clip remaining values to [-128, ..., 127]
	for (int i = 1; i < 64; i++) {
		int v1 = CLIP[256 + in[i] * QUANTIZER[i] / 32768];
		no_fec[i] = v1;

	}

}

/**
 * @brief	Quantize block (with one FEC level)
 */
void CVid_Quantizer_HiFEC::quantizeBlock(int *in, int *no_fec, int *lo_fec) {

	// first value should stay uncliped
	int v = in[0] * QUANTIZER[0] / 32768;

	no_fec[0] = v;
	lo_fec[0] = v / 4;

	// clip remaining values to [-128, ..., 127]
	for (int i = 1; i < 64; i++) {
		int v1 = CLIP[256 + in[i] * QUANTIZER[i] / 32768];
		no_fec[i] = v1;

		int v2 = v1 / 4;
		lo_fec[i] = v2;

	}

}

/**
 * @brief	Quantize block (with two FEC levels)
 */
void CVid_Quantizer_HiFEC::quantizeBlock(int *in, int *no_fec, int *lo_fec, int *hi_fec) {

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

/**
 * @brief	Set the number of FEC levels.
 *
 * @param levels	Number of FEC levels. Allowed values are 0 (no FEC),
 * 					1 (one FEC level), 2 (two FEC levels)
 */
void CVid_Quantizer_HiFEC::setFecLevels(int levels)
{
	assert(levels >= 0 && levels < 3); // [0..2]

	fec_levels = levels;
}

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
