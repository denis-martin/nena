//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#ifndef REBOOST_TLV_HPP__
#define REBOOST_TLV_HPP__

#include<memory.h>
#include<iostream>
#include<stdint.h>

#include "tlv_packer.hpp"
#include "../shared_buffer.hpp"
#include "../message.hpp"

using namespace std;

namespace reboost {

typedef size_t tlv_length_t;
typedef size_t tlv_type_t;

/**
 * A simple type-length-value store based on a shared buffer.
 *
 * @author Sebastian Mies <mies@reboost.org>
 */
class tlv_t : public shared_buffer_t {
private:
	tlv_type_t type_;
	typedef shared_buffer_t super;

public:
	/// base types
	enum base_types_t {
		NIL = 0, 			//!< empty type
		COMPRESSED = 1,		//!< compressed TLV inside
		FALSE = 2, 			//!< boolean false type
		TRUE = 3,  			//!< boolean true type
		UINT = 4, 			//!< fixed-size unsigned integer
		SINT = 5, 			//!< fixed-size signed integer
		VINTPOS = 6, 		//!< variable-size positive integer
		VINTNEG = 7, 		//!< variable-size negative integer
		FLOAT = 8, 			//!< floating point number
		TEXT = 9,			//!< text string
		ARRAY = 10,			//!< an array of same length and type TLVs
		LIST = 11, 			//!< a list of arbitrary TLVs
		RES1 = 12,			//!< reserved
		RES2 = 13,			//!< reserved
		RES3 = 14,	 		//!< reserved
		RES4 = 15, 			//!< reserved
		USER = 16  			//!< first user type
	};

	// nil tlv
	static const tlv_t nil;
	static const tlv_t truetlv;
	static const tlv_t falsetlv;

	// self-type
	typedef tlv_t self;

	/// create NIL-tlv
	inline tlv_t() :
		shared_buffer_t(), type_(NIL) {}

	/// copy constructor
	inline tlv_t(const self& rhs) :
		shared_buffer_t(rhs), type_(rhs.type_) {}

	/// create tlv from plain binary data
	inline tlv_t( tlv_type_t type_, tlv_length_t length, const char* value) :
		shared_buffer_t(value,length), type_(type_) {}

	/// clone data from a normal buffer
	inline tlv_t(tlv_type_t type_, const buffer_t& rhs) :
		shared_buffer_t(rhs), type_(type_) {}

	/// use data from a shared buffer
	inline tlv_t(tlv_type_t type_, const shared_buffer_t& rhs) :
		shared_buffer_t(rhs), type_(type_) {}

	/// special types: create tlv from string
	inline tlv_t(string str) :
		shared_buffer_t(str.c_str()), type_(TEXT) {}

	/// special types: create tlv from string
	inline tlv_t(const char* str) :
		shared_buffer_t(str), type_(TEXT) {}

	/// special types: create tlv from boolean
	inline tlv_t(bool boolean) :
		shared_buffer_t(), type_(boolean ? TRUE:FALSE) {}

	/// special types: create tlv from float
	inline tlv_t(float number) :
		shared_buffer_t(sizeof(float)), type_(FLOAT) {
		memcpy(mutable_data(),&number,sizeof(float));
	}

	/// special types: create tlv from double
	inline tlv_t(double number) :
		shared_buffer_t(sizeof(double)), type_(FLOAT) {
		memcpy(mutable_data(),&number,sizeof(double));
	}

	/// special types: create tlv from unsigned integer
	inline static tlv_t uint( uintmax_t value, int length = 0 ) {
		tlv_t tlv;
		// get length
		tlv.type_ = UINT;
		if (length==0) {
			tlv.type_ = VINTPOS;
			for (length=sizeof(uintmax_t)-1;
					length >= 0 && (value >> (length*8))==0; length--);
			length++;
		}
		tlv = shared_buffer_t(length);
		for (int i=0; i<length; i++) tlv[i] = (value >> (i*8)) & 0xFF;
		return tlv;
	}

	/// special types: create tlv from signed integer
	inline static tlv_t sint( intmax_t value, int length = 0 ) {
		if (length == 0) {
			tlv_t tlv = uint(abs(value),length);
			if (value<0) tlv.type_ = VINTNEG;
			return tlv;
		} else {
			tlv_t tlv = uint((uintmax_t)value,length);
			tlv.type_ = SINT;
			return tlv;
		}
	}

	/// compress tlv using miniz
	void compress();

	/// decompress tlv using miniz
	void uncompress();

	/// assign tlv
	inline self& operator=(const self& rhs) {
		shared_buffer_t::operator =(rhs);
		type_ = rhs.type_;
		return (*this);
	}

	/// assign buffer
	inline self& operator=(const shared_buffer_t& rhs) {
		shared_buffer_t::operator =(rhs);
		return (*this);
	}

	/// return sub-buffer.
	inline self operator()(bsize_t index, bsize_t size = 0) const {
		self n(*this);
		n.data_ += index;
		if (size == 0) n.size_ -= index;
		else n.size_ = size;
		return (n);
	}

	/// returns true, if this tlv has a NIL type
	inline bool is_nil() const {
		return type_ == NIL;
	}

	/// returns the type
	inline tlv_type_t type() const {
		return type_;
	}

	/// returns a hash value of this buffer using the ELF hash
	inline size_t hash() const {
		return buffer_t::hash() + type();
	}

	/// compare two tlvs
	inline int compare_to(const self& rhs) const {
		int cb = buffer_t::compare_to(rhs);
		if (cb==0 && rhs.type_ == type_) return 0;
		return cb;
	}

	/// convenience
	inline bool operator==(const self& rhs) const {
		return (compare_to(rhs) == 0);
	}
	inline bool operator!=(const self& rhs) const {
		return (compare_to(rhs) != 0);
	}
	inline bool operator<(const self& rhs) const {
		return (compare_to(rhs) < 0);
	}
	inline bool operator<=(const self& rhs) const {
		return (compare_to(rhs) <= 0);
	}
	inline bool operator>(const self& rhs) const {
		return (compare_to(rhs) > 0);
	}
	inline bool operator>=(const self& rhs) const {
		return (compare_to(rhs) >= 0);
	}



	//-------------------------------------------------------------------------

	/// returns the size of the packed tlv
	inline size_t pack_size() const {
		return tlv_packer::field_size(type(), size())+size();
	}

	/// pack tlv to a buffer
	inline size_t pack_to( uint8_t* buf ) const {
		size_t i = tlv_packer::set(buf, type(), size());
		memcpy(buf + i, data(), size() );
		return size() + i;
	}

	/// pack tlv to one shared buffer
	inline shared_buffer_t pack() const {
		shared_buffer_t buf(pack_size());
		pack_to(buf.mutable_data());
		return buf;
	}

	//-------------------------------------------------------------------------

	/// returns true, if the buffer contains a valid tlv
	inline bool match( const shared_buffer_t& buf ) const {
		return true;
	}

	/// unpack the tlv from a shared buffer and sets the size of the tlv
	static tlv_t unpack( const shared_buffer_t& buf, size_t& size ) {
		const boctet_t* data = buf.data();
		size_t tfl_size=tlv_packer::field_size(data);
		size = tfl_size + tlv_packer::length(data);
		return tlv_t(tlv_packer::type(data), buf(tfl_size, tlv_packer::length(data)));
	}

	/// unpack the tlv from a shared buffer
	static tlv_t unpack( const shared_buffer_t& buf ) {
		size_t s; return unpack(buf,s);
	}
};

/// stream operator
ostream& operator<<(ostream& os, const tlv_t& tlv);

/// message operators
inline message_t& operator<<(message_t& msg, const tlv_t& tlv) {
	msg.push_back( tlv.pack() );
	return msg;
}
inline message_t operator>>(const message_t& msg, tlv_t& tlv) {
	size_t sz=0;
	tlv = tlv_t::unpack( msg.at(0), sz );
	return msg(sz);
}

}

/// boost hash
namespace boost {
inline size_t hash_value(const reboost::tlv_t& tlv) {
	return tlv.hash();
}}

#endif /* TLV_HPP_ */

