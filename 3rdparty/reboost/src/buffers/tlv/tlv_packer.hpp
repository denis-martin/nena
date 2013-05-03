//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#ifndef REBOOST_TLV_PACKER_HPP__
#define REBOOST_TLV_PACKER_HPP__

#include <iostream>
#include <stdint.h>

using namespace std;

namespace reboost {

/**
 * A simple packed type-length encoder.
 *
 * It uses the following encoding rules:
 *
 * 8-bit  = 0TTT|TTLL
 *          (TTTTT=type, LL=log2 length-1 or zero)
 *
 * 16-bit = 10TT|TTTT|LLLLLLLLL
 *          (TTTTT=type, LLLLLLLL=length)
 *
 * long   = 11AA|ABBB ...
 *          (AAA=type-field size-1, BBB=length-field size-1)
 *
 * @author Sebastian Mies <mies@reboost.org>
 */
class tlv_packer {
private:
	inline static size_t bytes_of( size_t value ) {
		if (value==0) return 0;
		if ((value&0xFF000000)!=0) return 4;
		if ((value&0x00FF0000)!=0) return 3;
		if ((value&0x0000FF00)!=0) return 2;
		if ((value&0x000000FF)!=0) return 1;
		return 0;
	}

public:
	/// returns the size of the encoded type-length-field
	inline static size_t field_size( const uint8_t* tl ) {
		if ((tl[0] & 0x80)==0) return 1;
		if ((tl[0] & 0xC0)==0x80) return 2;
		return 1+(tl[0]&7)+((tl[0]>>3)&7);
	}

	/// returns the size of the encoded type-length-field
	inline static size_t field_size( size_t type, size_t length ) {
		if (type < 32 && ((151 >> length)&1)!=0) return 1;
		if (type < 64 && length < 256) return 2;
		return bytes_of(type)+bytes_of(length)+1;
	}

	/// returns the type
	inline static size_t type( const uint8_t* tl ) {
		if ((tl[0] & 0x80)==0) return tl[0] >> 2;
		if ((tl[0] & 0xC0)==0x80) return tl[0] & 0x3F;
		size_t _type = 0;
		for (int i=0; i<((tl[0]>>3) & 7); i++) _type |= ((size_t)tl[i+1]) << (i*8);
		return _type;
	}

	/// returns the length
	inline static size_t length( const uint8_t* tl ) {
		if ((tl[0] & 0x80)==0) return ((tl[0]&3)==0) ? 0 : (1 << ((tl[0]&3)-1));
		if ((tl[0] & 0xC0)==0x80) return tl[1];
		size_t _len = 0; size_t ofs = ((tl[0]>>3) & 7)+2;
		for (int i=0; i<(tl[0] & 7); i++) _len |= ((size_t)tl[i+ofs]) << (i*8);
		return _len;
	}

	/// get the type and length and return size
	inline static size_t get( const uint8_t* tl, size_t& type_, size_t& length_ ) {
		type_ = type(tl); length_ = length(tl);
		return field_size(tl);
	}

	/// set the type and length and return size
	inline static size_t set( uint8_t* tl, size_t type, size_t length) {
		// 8 bit type-length (0TTT|TTLL)
		if (type < 32 && ((151 >> length)&1)!=0) {
			// 0011|0000|0010|0001|0100
			tl[0]=(type << 2)|((0x0324 >> (length*2)) & 3);
			return 1;
		}
		// 16 bit type-length (10TT|TTTT|LLLLLLLLL)
		if (type < 64 && length < 256) {
			tl[0]=0x80 | type;
			tl[1]=length;
			return 2;
		}
		// var type-length (11FA|AABB)
		size_t a = bytes_of(type), b = bytes_of(length);
		tl[0] = 0xC0 | (a << 3) | b;
		for (size_t i=0; i<a; i++) { tl[i + 1] = type & 255; type >>= 8; }
		for (size_t i=0; i<b; i++) { tl[i + a + 2] = length & 255; length >>=8; }
		return a+b+1;
	}

	/// convert to string
	static string to_string( const uint8_t* tl );

	/// test class
	static bool test();
};

}

#endif
