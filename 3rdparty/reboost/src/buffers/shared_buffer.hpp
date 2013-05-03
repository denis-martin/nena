//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#ifndef REBOOST_SHARED_BUFFER_HPP_
#define REBOOST_SHARED_BUFFER_HPP_

#include <cstring>
#include <boost/shared_ptr.hpp>

#include "buffer.hpp"

namespace reboost {

/**
 * A simple shared buffer.
 *
 * Important: if not shared, a buffer is writable. After the buffer is shared,
 * the buffer is immutable. It uses shared_ptr/default allocators and prints
 * error messages to <tt>cerr</tt> if buffers leaked at the end of the
 * program.
 *
 * @author Sebastian Mies <mies@reboost.org>
 */
class shared_buffer_t: public buffer_t {
	typedef shared_buffer_t self;
private:
	static void onexit();
	static size_t init();
	static size_t allocated_buffers;

	class deleteable_buffer: public buffer_t {
	public:
		deleteable_buffer(bsize_t size) :
			buffer_t(size) {
			allocated_buffers++;
		}
		~deleteable_buffer() {
			if (!is_null()) delete[] data_;
			allocated_buffers--;
		}
	};
	boost::shared_ptr<deleteable_buffer> parent;

public:
	/// create null-buffer
	inline shared_buffer_t() :
		buffer_t(), parent() {
	}

	/// create a shared buffer of a specific size
	inline shared_buffer_t(bsize_t size) :
		buffer_t(), parent(new deleteable_buffer(size)) {
		buffer_t::operator=(*parent);
	}

	/// create shared buffer from string
	inline shared_buffer_t(const char* string) :
		buffer_t(), parent() {
		parent.reset(new deleteable_buffer(strlen(string)));
		memcpy(parent->mutable_data(),string,parent->size());
		data(parent->mutable_data()); size(parent->size());
	}

	/// create shared buffer from buffer
	inline shared_buffer_t(const char* buffer, bsize_t size) :
		buffer_t(), parent(new deleteable_buffer(size)) {
		memcpy(parent->mutable_data(), buffer, parent->size());
		data(parent->mutable_data()); this->size(parent->size());
	}

	/// clone data from a normal buffer
	inline shared_buffer_t(const buffer_t& rhs) :
		buffer_t(), parent(new deleteable_buffer(rhs.size())) {
		buffer_t::operator=(*parent);
		rhs.copy_to(*parent, 0);
	}

	/// copy constructor
	inline shared_buffer_t(const self& rhs) :
		buffer_t(rhs), parent(rhs.parent) {
	}

	/// copy a buffer pointer and size.
	inline self& operator=(const self& rhs) {
		parent = rhs.parent;
		buffer_t::operator=(rhs);
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

	/// returns the number of references
	inline size_t use_count() const {
		return (parent.use_count());
	}

	/// returns true, if this is root buffer
	inline bool is_root() const {
		return ((parent.get() != NULL) && (this->data() == parent->data()) && (
				this->size() == parent->size()));
	}

	/// returns true, if the buffer is not shared
	inline bool is_unique() const {
		return parent.unique();
	}

	/// makes the buffer unique / not shared by copying the contents
	inline void make_unique() {
		if (is_unique()) return;
		boost::shared_ptr<deleteable_buffer> old = parent;
		parent.reset(new deleteable_buffer(old->size()));
		buffer_t::data(parent->mutable_data());
		old->copy_to(*parent,0);
	}

	/// resize the shared buffer (and make unique)
	inline void resize( bsize_t new_size ) {
		boost::shared_ptr<deleteable_buffer> old = parent;
		parent.reset(new deleteable_buffer(new_size));
		buffer_t::data(parent->mutable_data());
		(*old)(0,new_size).copy_to(*parent,0);
		size_ = new_size;
	}

	/// returns a pointer to mutable data, if shared_buffer is not shared yet.
	inline boctet_t * mutable_data() {
		assert(parent.unique());
		return buffer_t::mutable_data();
	}

	/// drop reference to buffer
	inline void reset() {
		parent.reset();
		data(NULL);
		size(0);
	}

	/// concatenate buffers
	self operator+( const buffer_t& rhs ) const {
		self nb(size()+rhs.size());
		this->copy_to(nb,0);
		rhs.copy_to(nb,size());
		return nb;
	}
};

} /* namespace reboost */

#endif /* REBOOST_SHARED_BUFFER_HPP_ */
