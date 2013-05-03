//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#ifndef REBOOST_TLV_ARRAY_HPP__
#define REBOOST_TLV_ARRAY_HPP__

#include "tlv.hpp"
#include <vector>
#include <iostream>

namespace reboost {

using namespace std;
using namespace boost;

/**
 * An type-length-value array having the same types.
 *
 * @author Sebastian Mies
 */
class tlv_array {
private:
	tlv_type_t type;
	tlv_length_t length;

private:
	shared_buffer_t buf;
	size_t num_items;

public:
	typedef tlv_array self;

	class assign_tlv_t {
	private:
		self& array;
		const size_t index;
	public:
		inline assign_tlv_t( self& self, size_t index ) :
			array(self), index(index) {}
		inline operator tlv_t () {
			return tlv_t( array.type, array.buf(index * array.length, array.length) );
		}
		inline assign_tlv_t& operator=( const tlv_t& tlv ) {
			assert( tlv.type() == array.length && tlv.size() == array.type );
			memcpy(array.buf.mutable_data()+ index * array.length, tlv.data(), array.length);
			return *this;
		}
		inline assign_tlv_t& operator=( uintmax_t integer ) {
			assert(array.type == tlv_t::UINT || array.type == tlv_t::SINT);
			uint8_t* b = array.buf.mutable_data() + index * array.length;
			for (size_t i=0; i<array.length; i++)
				b[i] = (integer >> (i*8)) & 0xFF;
			return *this;
		}
	};

public:
	/// create an empty, undefined array
	inline tlv_array() : type(tlv_t::NIL), length(0), buf(), num_items(0) {}

	/// create array with the given type/length and number of items
	inline tlv_array( tlv_type_t type, tlv_length_t length, size_t num_items = 0) :
		type(type), length(length),
		buf(num_items * length), num_items(num_items) {}

	/// copy array
	inline tlv_array( const self& array ) :
			type(array.type), length(array.length),
			buf(array.buf), num_items(array.num_items) {}

	/// duplicate array (do not share underlaying buffer)
	inline tlv_array dup() const {
		tlv_array new_array = *this;
		new_array.buf.make_unique();
		return new_array;
	}

	/// return the size of the array
	inline size_t size() const {
		return num_items;
	}

	/// re-sizes the array
	inline void resize( size_t new_size ) {
		num_items = new_size;
		size_t new_buf_size = num_items * length;
		if (new_buf_size > buf.size() || (new_buf_size+16) < buf.size()) {
			size_t s = ((num_items * length)&(~15))+16;
			if (s!=buf.size()) buf.resize(s);
		}
	}

	/// equalizes the size of the internal buffer to the array size
	inline void compact() {
		size_t new_buf_size = num_items*length;
		if (new_buf_size!=buf.size()) buf.resize(new_buf_size);
	}

	/// add a tlv
	inline void push_back( const tlv_t& tlv ) {
		resize(num_items+1);
		(*this)[num_items-1] = tlv;
	}

	/// return a const-tlv at the given index
	inline const tlv_t operator[] ( size_t index ) const {
		return tlv_t( type, buf(index * length, length) );
	}

	/// return an assignable item at the given index
	inline assign_tlv_t operator[] ( size_t index ) {
		return assign_tlv_t(*this, index);
	}

	//-------------------------------------------------------------------------

	/// returns the packed size of the list
	size_t pack_size() const {
		size_t itm_tls = tlv_packer::field_size( type, length );
		return tlv_packer::field_size( tlv_t::ARRAY, length * num_items + itm_tls )+
			itm_tls + num_items * length;
	}

	/// returns a tlv of the list
	size_t pack_to( uint8_t* buf ) const {
		size_t itm_tls = tlv_packer::field_size( type, length );
		size_t arr_tls = tlv_packer::set( buf, tlv_t::ARRAY, length*num_items + itm_tls );
		tlv_packer::set( buf+arr_tls, type, length );
		memcpy( buf+arr_tls+itm_tls, this->buf.data(), num_items*length );
		return pack_size();
	}

	/// pack a shared buffer
	shared_buffer_t pack() const {
		shared_buffer_t buf(pack_size());
		pack_to(buf.mutable_data());
		return buf;
	}

	//-------------------------------------------------------------------------

	/// returns true, if the buffer contains a valid list
	inline static bool match( const buffer_t& buf ) {
		return tlv_packer::type(buf.data()) == tlv_t::ARRAY;
	}

	/// unpack a tlv list
	inline static tlv_array unpack( const shared_buffer_t& buf, size_t& size ) {
		tlv_length_t arr_len;
		tlv_type_t arr_type;
		size_t arr_header = tlv_packer::get( buf.data(), arr_type, arr_len );
		size = arr_header + arr_len;
		assert(arr_type == tlv_t::ARRAY);
		return unpacki(buf(arr_header,arr_len));
	}

	inline static tlv_array unpacki( const shared_buffer_t& buf ) {
		tlv_length_t items_len;
		tlv_type_t items_type;
		size_t items_header =  tlv_packer::get( buf.data(), items_type, items_len );
		size_t items_num = (buf.size()-items_header) / items_len;
		tlv_array arr(items_type, items_len, items_num);
		arr.buf = buf(items_header,(buf.size()-items_header));
		return arr;
	}

	inline static tlv_array unpack( const shared_buffer_t& buf ) {
		size_t s = 0; return unpack(buf,s);
	}
};

/// stream operator
std::ostream& operator<<( std::ostream& os, const tlv_array& arr );

/// message operators
inline message_t& operator<<(message_t& msg, const tlv_array& arr) {
	msg.push_back( arr.pack() );
	return msg;
}
inline message_t operator>>(const message_t& msg, tlv_array& arr) {
	size_t sz=0;
	arr = tlv_array::unpack( msg.at(0), sz );
	return msg(sz);
}

}

#endif /* TLV_ARRAY_HPP_ */
