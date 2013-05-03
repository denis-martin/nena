//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#ifndef REBOOST_TLV_LIST_HPP__
#define REBOOST_TLV_LIST_HPP__

#include "tlv.hpp"
#include <vector>
#include <iostream>
#include <boost/unordered_set.hpp>

namespace reboost {

using namespace std;
using namespace boost;

/**
 * A list of arbitrary TLVs. If all items of the list have the same type
 * and length consider using <tt>tlv_array</tt>, it has a better performance
 * and smaller memory footprint.
 *
 * @author Sebastian Mies <mies@reboost.org>
 */
class tlv_list : public vector<tlv_t> {
private:
	typedef vector<tlv_t> super;

public:
	/// construct an empty list
	tlv_list() : super() {}

	/// copy constructor
	tlv_list( const tlv_list& rhs ) : super(rhs) {}

	/// list with a single element
	tlv_list( const tlv_t& rhs ) : super() {
		push_back(rhs);
	}

	/// returns the types comprised in the list
	inline unordered_set<tlv_type_t> types() const {
		unordered_set<tlv_type_t> tps;
		for (super::const_iterator i = begin(); i!=end(); i++)
			tps.insert(i->type());
		return tps;
	}

	//-------------------------------------------------------------------------

	/// returns the packed size of the list
	size_t pack_size() const {
		size_t fields_size = 0, content_size = 0;
		bool same_type = true;
		for (super::const_iterator i = begin(); i!=end(); i++) {
			if (i->type() != at(0).type() || i->size()!=at(0).size())
				same_type = false;
			fields_size += tlv_packer::field_size(i->type(), i->size());
			content_size += i->size();
		}
		if (same_type) {
			size_t field_size = tlv_packer::field_size(at(0).type(), at(0).size());
			return content_size + field_size +
				tlv_packer::field_size(tlv_t::ARRAY, content_size + field_size);
		} else
			return content_size + fields_size +
				tlv_packer::field_size(tlv_t::LIST, content_size + fields_size);
	}

	/// returns a tlv of the list
	size_t pack_to( uint8_t* buf ) const {
		uint8_t* old_buf = buf;

		// check if the list is of single type and tlv's have the same length
		tlv_type_t type = tlv_t::ARRAY;
		for (super::const_iterator i = begin(); i!=end(); i++) {
			if (i->type() != at(0).type() || i->size()!=at(0).size()) {
				type = tlv_t::LIST;
				break;
			}
		}

		// pack a list with a single type
		if (type == tlv_t::ARRAY) {
			size_t size = tlv_packer::field_size(at(0).type(), at(0).size()) +
					this->size() * at(0).size();
			buf += tlv_packer::set(buf, type, size);
			buf += tlv_packer::set(buf, at(0).type(), at(0).size());
			for (super::const_iterator i = begin(); i!=end(); i++) {
				memcpy(buf, i->data(), i->size());
				buf += i->size();
			}
		} else

		// pack a list with a multiple types
		if (type == tlv_t::LIST) {

			// calculate size
			size_t sz = 0;
			for (super::const_iterator i = begin(); i!=end(); i++) {
				sz += tlv_packer::field_size(i->type(), i->size());
				sz += i->size();
			}
			buf += tlv_packer::set(buf, type, sz);

			// add items
			for (super::const_iterator i = begin(); i!=end(); i++) {
				buf += tlv_packer::set(buf, i->type(), i->size());
				memcpy(buf, i->data(), i->size());
				buf += i->size();
			}
		}

		return (size_t)(buf-old_buf);
	}

	/// pack tlv to one shared buffer
	inline shared_buffer_t pack() const {
		shared_buffer_t buf(pack_size());
		pack_to(buf.mutable_data());
		return buf;
	}
	//-------------------------------------------------------------------------

	/// returns true, if the buffer contains a valid list
	inline bool match( const shared_buffer_t& buf ) const {
		tlv_type_t type = tlv_packer::type(buf.data());
		return (type == tlv_t::ARRAY || type == tlv_t::LIST);
	}

	/// unpack a tlv list
	inline static tlv_list unpack( const shared_buffer_t& buf, size_t& size ) {
		tlv_type_t list_type;
		tlv_length_t list_length;
		size_t index = tlv_packer::get( buf.data(), list_type, list_length );
		size = index+list_length;
		return unpacki(list_type, buf(index,list_length));
	}

	inline static tlv_list unpacki( tlv_type_t list_type, const shared_buffer_t& buf ) {
		tlv_list list;
		tlv_length_t list_length = buf.size();
		size_t index = 0;
		const uint8_t* b = buf.data();


		// single type-length list
		if (list_type == tlv_t::ARRAY) {
			size_t item_type, item_length;
			size_t tl_field = tlv_packer::get( b, item_type, item_length );
			index += tl_field;
			list_length -= tl_field;
			while (list_length >= item_length) {
				list.push_back( tlv_t( item_type, buf(index,item_length) ) );
				index += item_length;
				list_length -= item_length;
			}
		} else

		// multiple type-length list
		if (list_type == tlv_t::LIST) {
			while (true) {
				size_t item_type, item_length;
				size_t tl_field = tlv_packer::get( b+index, item_type, item_length );
				index += tl_field;
				list.push_back( tlv_t( item_type, buf(index,item_length) ) );
				index += item_length;
				if (list_length <= (item_length+tl_field)) break;
				list_length -= (item_length+tl_field);
			}
		}
		return list;
	}

	inline static tlv_list unpack( const shared_buffer_t& buf ) {
		size_t s; return unpack(buf,s);
	}

};

/// stream operator
std::ostream& operator<<( std::ostream& os, const tlv_list& set );

/// operator
inline tlv_list& operator<<(tlv_list& lst, const tlv_t& tlv) {
	lst.push_back(tlv);
	return lst;
}

inline tlv_list operator+(const tlv_t& lhs, const tlv_t& rhs) {
	tlv_list lst;
	lst.push_back(lhs);
	lst.push_back(rhs);
	return lst;
}

inline tlv_list operator+( tlv_list lst, const tlv_t& rhs ) {
	lst.push_back(rhs);
	return lst;
}

/// message operators
inline message_t& operator<<(message_t& msg, const tlv_list& lst) {
	msg.push_back( lst.pack() );
	return msg;
}
inline message_t operator>>(const message_t& msg, tlv_list& lst) {
	size_t sz=0;
	lst = tlv_list::unpack( msg.at(0), sz );
	return msg(sz);
}

}

#endif
