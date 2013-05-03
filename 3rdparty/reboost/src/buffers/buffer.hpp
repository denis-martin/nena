//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#ifndef BUFFER_HPP_
#define BUFFER_HPP_

#include<cassert>
#include<iostream>

namespace reboost {

/// buffer size and octet
typedef size_t bsize_t;
typedef unsigned char boctet_t;

/**
 * A simple buffer.
 *
 * @author Sebastian Mies <mies@cpptools.org>
 */
class buffer_t {
	typedef buffer_t self;
protected:
	bsize_t size_;
	boctet_t * data_;

public:
	/// a buffer of zero size, pointing to NULL
	inline buffer_t() :
		size_(0), data_((boctet_t*) 0) {
	}

	/// copy constructor
	inline buffer_t(const self& rhs) :
		size_(rhs.size_), data_(rhs.data_){
	}

	/// allocate a buffer of a certain size
	inline buffer_t(bsize_t size) :
		size_(size), data_(new boctet_t[size]) {
	}

	/// convenience constructor
	inline buffer_t(bsize_t size, boctet_t* data) :
		size_(size), data_(data) {
	}

	/// interpret string as buffer (maximal size 64K)
	inline explicit buffer_t(const char* string) :
		size_(0), data_((boctet_t*) string) {
		while (string[size_] != 0 && size_ < 64000)
			size_++;
	}

	/// clone a buffer
	inline self clone() const {
		buffer_t n(this->size_);
		for (bsize_t i = 0; i < size_; i++)
			n.data_[i] = this->data_[i];
		return n;
	}

	/// copy a buffer pointer and size.
	inline self& operator=(const self& rhs) {
		this->size_ = rhs.size_;
		this->data_ = rhs.data_;
		return *this;
	}

	/// return sub-buffer.
	inline self operator()(bsize_t index, bsize_t size = 0) const {
		self n(*this);
		n.data_ += index;
		if (size == 0) n.size_ -= index;
		else n.size_ = size;
		return n;
	}

	/// returns a data element at given index
	inline boctet_t operator[](bsize_t index) const {
		return data_[index];
	}

	/// returns a data element at given index
	inline boctet_t& operator[](bsize_t index) {
		return data_[index];
	}

	/// assigns contents to another buffer
	inline const self& copy_to( self& buf, bsize_t index = 0 ) const {
		assert(index + size_ <= buf.size_);
		const boctet_t *src=data_;
		boctet_t* dst=buf.data_+index;
		for (bsize_t i=0; i<size_; i++, src++, dst++ ) *dst = *src;
		return *this;
	}

	/// convert to pointer of an immutable type
	template<class T> inline const T* cast_to() {
		return (T*)data_;
	}

	/// set buffer size
	inline void size( bsize_t size ) {
		size_ = size;
	}


	/// returns the size in octets
	inline bsize_t size() const {
		return size_;
	}

	/// sets the data pointer
	inline void data( boctet_t* data ) {
		data_ = data;
	}

	/// returns a pointer to immutable data
	inline const boctet_t * data() const {
		return data_;
	}

	/// returns a pointer to mutable data
	inline boctet_t * mutable_data() {
		return data_;
	}

	/// returns a hash value of this buffer using the ELF hash
	inline size_t hash() const {
		unsigned h = 0, g;
		for (size_t i = 0; i < size_; i++) {
			h = (h << 4) + data_[i];
			g = h & 0xf0000000L;
			if (g != 0) h ^= g >> 24;
			h &= ~g;
		}
		return (size_t) h;
	}

	/// returns true if data_ is null
	inline bool is_null() const {
		return data_ == NULL;
	}

	/// returns true if size is zero
	inline bool is_empty() const {
		return size_ == 0;
	}

	/// compare two buffers
	inline int compare_to(const self& rhs) const {
		if (rhs.data_ == data_ && rhs.size_ == size_) return 0;
		if (size_ < rhs.size_) return -1;
		if (size_ > rhs.size_) return 1;
		for (bsize_t i = 0; i < size_; i++) {
			if (data_[i] > rhs.data_[i]) return 1;
			if (data_[i] < rhs.data_[i]) return -1;
		}
		return 0;
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
};

/// stream operator
std::ostream& operator<<( std::ostream& os, const buffer_t& buf );

}

/// boost hash
namespace boost {
inline size_t hash_value(const reboost::buffer_t& buf) {
	return buf.hash();
}}

#endif /* BUFFER_HPP_ */
