//-----------------------------------------------------------------------------
// Part of reboost (http://reboost.org).  Released under the
// BSD 2-clause license (http://www.opensource.org/licenses/bsd-license.php).
// Copyright 2012, Sebastian Mies <mies@reboost.org> --- All rights reserved.
//-----------------------------------------------------------------------------

#ifndef REBOOST_MESSAGE_HPP_
#define REBOOST_MESSAGE_HPP_

#include<boost/thread.hpp>
#include<boost/shared_ptr.hpp>
#include<cstring>

#include "shared_buffer.hpp"

namespace reboost {

/// message size type
typedef signed char mlength_t;

/// maximum number of buffers per message (default is 8)
const mlength_t message_max_buffers = (1L << 3);

//! A Copy-on-Write Message with Shared Buffers.
/**
 * A Copy-on-Write Message with Shared Buffers.
 *
 * A message holds a limited (defined by <code>message_max_buffers</code>)
 * number of shared buffers. One can add new buffers and messages in front and
 * at the end of a message. If the no. of buffers exceed
 * <code>message_max_buffers</code>, then the two smallest successive buffers
 * are compacted to one buffer.
 *
 * @author Sebastian Mies <mies@reboost.org>
 */
class message_t {
private:
	// read sub-message
	struct sub_message {
		message_t* msg;
		inline void operator()(shared_buffer_t buf) {
			msg->push_back(buf);
		}
	};

	// read from buffer
	struct read_buffer {
		boctet_t* buffer;
		inline void operator()(buffer_t buf) {
			memcpy((void*) buffer, (void*) buf.data(), buf.size());
			buffer += buf.size();
		}
	};

	// write to buffer
	struct write_buffer {
		const boctet_t* buffer;
		inline void operator()(buffer_t buf) {
			memcpy((void*) buf.data(), (void*) buffer, buf.size());
			buffer += buf.size();
		}
	};

public:
	/// Create a new message
	inline message_t() :
		imsg() {
	}

	/// Copy message
	inline message_t(const message_t& msg) :
		imsg(msg.imsg) {
		imsg->owner = NULL;
	}

	/// Linearize message
	inline operator shared_buffer_t() const {
		return linearize();
	}

	/// Assign another message
	inline message_t& operator=(const message_t& msg) {
		msg.imsg->owner = NULL;
		imsg = msg.imsg;
		return *this;
	}

	/// Adds a shared buffer of given site at the end
	inline shared_buffer_t& push_back( bsize_t size ) {
		shared_buffer_t b(size); push_back(b);
		return imsg->at(-1);
	}

	/// Adds a buffer at the end of the message
	inline void push_back(const shared_buffer_t& buf) {
		own().push_back(buf);
	}

	/// Adds a message at the end of the message
	inline void push_back(const message_t& msg) {
		own();
		for (mlength_t i = 0; i < msg.length(); i++)
			push_back(msg[i]);
	}

	/// Adds a shared buffer of given size at the front
	inline shared_buffer_t& push_front( bsize_t size ) {
		shared_buffer_t b(size); push_front(b);
		return imsg->at(0);
	}

	/// Adds a buffer at the front of the messsage
	inline void push_front(const shared_buffer_t& buf) {
		own().push_front(buf);
	}

	/// Adds a message at the end of the message
	inline void push_front(const message_t& msg) {
		own();
		for (mlength_t i = msg.length() - 1; i >= 0; i--)
			push_front(msg[i]);
	}

	/// Removes a buffer from the end of the message
	inline shared_buffer_t pop_back() {
		return own().pop_back();
	}

	/// Removes a buffer from the beginning of this message.
	inline shared_buffer_t pop_front() {
		return own().pop_front();
	}

	/// Returns the size of the message in bytes (or octets).
	inline size_t size() const {
		size_t s = 0;
		for (mlength_t i = 0; i < length(); i++)
			s += operator[](i).size();
		return (s);
	}

	/// Returns the number of buffers inside this message.
	inline mlength_t length() const {
		return imsg.get() ? imsg->length : 0;
	}

	/// Returns the buffer at the given index.
	inline shared_buffer_t& operator[](mlength_t idx) {
		return at(idx);
	}

	/// Returns the buffer at the given index.
	inline shared_buffer_t& at(mlength_t idx) {
		return imsg->at(idx);
	}

	/// Returns the constant buffer at the given index.
	inline const shared_buffer_t& operator[](mlength_t idx) const {
		return at(idx);
	}

	/// Returns the buffer at the given index
	inline const shared_buffer_t& at(mlength_t idx) const {
		return imsg->at(idx);
	}

	/// Iterates over a partial set of buffers.
	template<typename T>
	inline void foreach(const T& work, size_t index_ = 0, size_t size_ = 0) const {
		T op = work;
		if (size_ == 0) size_ = size() - index_;

		// get first buffer
		mlength_t f = 0;
		size_t pf = 0;
		for (; f < imsg->length && (pf + at(f).size()) <= index_;
				pf += at(f).size(), f++);
		// get last buffer
		mlength_t l = f;
		size_t pl = pf;
		for (; l < imsg->length && (pl + at(l).size()) < (index_ + size_);
				pl += at(l).size(), l++);

		// same buffer? yes-> get sub-buffer
		if (l == f) op(at(l)(index_ - pf, size_));
		else { // no-> get sub-buffers :)
		    size_t copiedLength = 0;
			op(at(f)(index_ - pf));
			copiedLength += at(f).size() - (index_ - pf);
			for (mlength_t i = f + 1; i < l; i++) {
				op(at(i));
				copiedLength += at(i).size();
			}
			op(at(l)(0, size_ - copiedLength));
		}
	}

	/// Read bytes (gather).
	inline void read(boctet_t* mem, size_t idx = 0, size_t size_ = 0) const {
		struct read_buffer rb = { mem };
		foreach(rb, idx, size_);
	}

	/// write bytes
	inline void write(const boctet_t* mem, size_t idx = 0, size_t size_ = 0) {
		struct write_buffer wb = { mem };
		foreach(wb, idx, size_);
	}

	/// Read an arbitrary, binary object.
	template<class T>
	inline T read(size_t index) {
		T obj;
		read((boctet_t*) &obj, index, sizeof(T));
		return obj;
	}

	/// Write an arbitrary, binary object.
	template<class T>
	inline void write(const T& value, size_t index) {
		write((boctet_t*) &value, index, sizeof(T));
	}

	/// Calculate a (ELF-like) hash.
	inline size_t hash() const {
		size_t h = 0;
		for (mlength_t i = 0; i < length(); i++)
			h += at(i).hash() * (i + 1);
		return h;
	}

	/// Returns a sub-message.
	message_t operator()(size_t index, size_t size = 0) const {
		message_t m;
		struct sub_message sm = { &m };
		foreach(sm, index, size);
		return m;
	}

	/// Linearizes the complete/partial message into one shared buffer.
	inline shared_buffer_t linearize(size_t index = 0, size_t size_ = 0) const {
		shared_buffer_t b(size_ == 0 ? size() : size_);
		if (b.size() > 0)
			read(b.mutable_data(), index, size_);
		return b;
	}

private:
	class imsg_t {
	public:
		volatile message_t* owner;
		shared_buffer_t buffers[message_max_buffers];
		mlength_t index, length;
	public:
		inline imsg_t() :
			index(0), length(0) {
		}
		inline imsg_t(const imsg_t& imsg) :
			index(imsg.index), length(imsg.length) {
			for (mlength_t i = 0; i < length; i++)
				at(index + i) = imsg.at(index + i);
		}
		inline shared_buffer_t& at(mlength_t idx) {
			if (idx < 0) idx += length;
			return buffers[(idx + index) & (message_max_buffers - 1)];
		}
		inline const shared_buffer_t& at(mlength_t idx) const {
			if (idx < 0) idx += length;
			return buffers[(idx + index) & (message_max_buffers - 1)];
		}

		inline void push_back(const shared_buffer_t& buf) {
			if (buf.size() == 0) return;
			if (length == message_max_buffers) compact();
			at(length) = buf;
			length++;
		}

		inline void push_front(const shared_buffer_t& buf) {
			if (buf.size() == 0) return;
			if (length == message_max_buffers) compact();
			index--;
			length++;
			at(0) = buf;
		}

		inline shared_buffer_t pop_back() {
			shared_buffer_t& buf = at(-1);
			shared_buffer_t ret = buf;
			buf.reset();
			length--;
			return ret;
		}

		inline shared_buffer_t pop_front() {
			shared_buffer_t& buf = at(0);
			shared_buffer_t ret = buf;
			buf.reset();
			length--;
			index++;
			return ret;
		}

		/// compacts the buffers, so one more buffer is available
		inline void compact() {

			// find compacting candidate
			bsize_t min_size=~0, min_pos=0;
			for (mlength_t i=0; i<length; i++) {
				bsize_t c = at(i).size() + at(i+1).size();
				if (c < min_size || min_size == ~(bsize_t)0 ) {
					min_size = c;
					min_pos = i;
				}
			}

			// compact buffers
			shared_buffer_t nb(min_size);
			at(min_pos).copy_to( nb, 0 );
			at(min_pos+1).copy_to( nb, at(min_pos).size() );

			// move buffers and assign new buffer
			for (mlength_t i=min_pos+1; i<length; i++) at(i) = at(i+1);
			at(min_pos) = nb;

			length--;
		}
	};
	/// own a new message
	inline imsg_t& own() {
		if (imsg.get() != NULL && imsg->owner == this) return *imsg;
		if (imsg.get() == NULL) imsg = boost::shared_ptr<imsg_t>(new imsg_t());
		else imsg = boost::shared_ptr<imsg_t>(new imsg_t(*imsg));
		imsg->owner = this;
		return *imsg;
	}
	boost::shared_ptr<imsg_t> imsg;
};

inline message_t operator+(const message_t& lhs, const message_t& rhs) {
	message_t m = lhs;
	m.push_back(rhs);
	return m;
}

inline message_t operator+(const message_t& lhs, const buffer_t& rhs) {
	message_t m = lhs;
	m.push_back(rhs);
	return m;
}

inline message_t operator+(const shared_buffer_t& lhs, const message_t& rhs) {
	message_t m = rhs;
	m.push_front(lhs);
	return m;
}

inline message_t operator+(const shared_buffer_t& lhs,
	const shared_buffer_t& rhs) {
	message_t m;
	m.push_back(lhs);
	m.push_back(rhs);
	return m;
}

std::ostream& operator<<(std::ostream&, const message_t);

} /* namespace reboost */

#endif /* REBOOST_MESSAGE_HPP_ */
