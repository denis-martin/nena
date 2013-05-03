/** @file
 * messageBuffers.h
 *
 * @brief Message buffers for serialized messages
 *
 * (c) 2011 Institut fuer Telematik, Karlsruhe Institute of Technology (KIT), Germany
 *
 *  Created on: 28 Mar 2011
 *      Author: denis
 */

#ifndef MESSAGEBUFFERS_H_
#define MESSAGEBUFFERS_H_

#include "reboost/src/buffers/buffers.hpp"

#include "messages.h"

#include "debug.h"

#include <stddef.h> // for NULL definition
#include <stdint.h>
#include <sys/types.h>

// TODO: fix this include for FreeBSD/Windows
#include <netinet/in.h>

using namespace reboost;

class CMessageBuffer;

class IHeader
{
public:
	/**
	 * @brief 	Serialize all relevant data into a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const = 0;

	/**
	 * @brief 	De-serialize all relevant data from a byte buffer.
	 * 			Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> mbuf) = 0;

};

class CMessageBuffer : public IMessage
{
protected:
	message_t buffer;
	unsigned int cursor;

public:
	/**
	 * @brief Constructor
	 *
	 * @param buffer	Initial buffer (will be copied)
	 */
	CMessageBuffer(IMessageProcessor* from = NULL, IMessageProcessor* to = NULL, const Type type = t_outgoing,
			std::size_t bufferSize = 0) :
			IMessage(from, to, type), cursor(0)
	{
		className += "::CMessageBuffer";
		if (bufferSize > 0)
			this->buffer.push_back(shared_buffer_t(bufferSize));
	}

	/**
	 * @brief Constructor
	 *
	 * @param buffer	Initial buffer (will be copied)
	 */
	CMessageBuffer(buffer_t buffer) :
			IMessage(NULL, NULL, t_outgoing), cursor(0)
	{
		// copies initial buffer
		if (buffer.size() > 0)
			this->buffer.push_back(shared_buffer_t(buffer));
	}

	/**
	 * @brief Constructor
	 *
	 * @param buffer	Initial shared buffer
	 */
	CMessageBuffer(shared_buffer_t buffer) :
			IMessage(NULL, NULL, t_outgoing), cursor(0)
	{
		// copies initial buffer
		if (buffer.size() > 0)
			this->buffer.push_back(buffer);
	}

	/**
	 * @brief Constructor
	 *
	 * @param buffer	Initial shared buffer
	 */
	CMessageBuffer(message_t buffer) :
			IMessage(NULL, NULL, t_outgoing), buffer(buffer), cursor(0)
	{}

	/**
	 * @brief Constructor
	 *
	 * @param size	Initial buffer size
	 */
	CMessageBuffer(std::size_t size) :
			IMessage(NULL, NULL, t_outgoing), cursor(0)
	{
		// creates initial buffer
		if (size > 0) {
			shared_buffer_t buf(size);
			buffer.push_back(buf);
		}
	}

	/**
	 * @brief	Copy constructor
	 */
	CMessageBuffer(const CMessageBuffer& mbuf) :
					IMessage(mbuf.from, mbuf.to, mbuf.type),
					buffer(mbuf.buffer),
					cursor(mbuf.cursor)
	{
		/// copy properties
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
		for (pit = mbuf.properties.begin(); pit != mbuf.properties.end(); pit++)
			if (pit->second != NULL)
				properties[pit->first] = pit->second->clone();
	}

	/**
	 * @brief	Copy constructor (only the properties are copied)
	 */
	CMessageBuffer(std::size_t size, const CMessageBuffer& mbuf) :
		IMessage(NULL, NULL, mbuf.getType()),
		cursor(0)
	{
		// creates initial buffer
		if (size > 0) {
			shared_buffer_t buf(size);
			buffer.push_back(buf);
		}

        /// copy properties
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
		for (pit = mbuf.properties.begin(); pit != mbuf.properties.end(); pit++)
			if (pit->second != NULL)
				properties[pit->first] = pit->second->clone();
	}

	/**
	 * @brief Destructor.
	 */
	virtual ~CMessageBuffer()
	{
	}

	/**
	 * @brief	Reset buffer.
	 */
	virtual void reset()
	{
		while (buffer.length() > 0) {
			buffer.pop_front();
		}
		setFrom(NULL);
		setTo(NULL);
		setType(t_outgoing);
		setFlowState(boost::shared_ptr<CFlowState>());
		cursor = 0;
		properties.clear();
	}

	/**
	 * @brief	Creates a string with a hex representation of the buffer's
	 * 			content.
	 */
	std::string to_str(const bool pretty = true, const std::size_t count = 0)
	{
		boost::format fmt("%1$02x");
		std::string s, hs;
		std::size_t bytes = 0;
		std::size_t max = count > 0 ? count : buffer.size();
		for (mlength_t i = 0; i < buffer.length() && bytes < max; i++) {
			for (bsize_t j = 0; j < buffer.at(i).size() && bytes < max; j++, bytes++) {
				unsigned int ascii = (unsigned int) buffer.at(i)[j];
				s += (fmt % ascii).str();
				hs += (ascii >= 32 && ascii < 127) ? (char) ascii : '.';
				if (pretty) {
					if (!((bytes + 1) % 4)) {
						s += ' ';
						if (!((bytes + 1) % 8))
							hs += ' ';
					}
					if (!((bytes + 1) % 16)) {
						s += ' ' + hs;
						if (bytes + 1 < max)
							s += '\n';
						hs.clear();
					}
				}
			}
		}
		if (pretty && !hs.empty()) {
			while ((bytes + 1) % 16) {
				s += "  ";
				if (((bytes + 1) % 4) == 0)
					s += ' ';
				bytes++;
			}
			s += "  " + hs;
		}
		return s;
	}

	/**
	 * @brief	Return a size of all buffers.
	 */
	virtual std::size_t size() const
	{
		return buffer.size();
	}

	/**
	 * @brief	Return remaining size when push_ulong etc is used. Only needed
	 * 			when writing header values in a buffer of a predefined size.
	 *
	 * 			In case of pop_* the following holds getRemainingSize() == size().
	 */
	virtual std::size_t getRemainingSize() const
	{
		return buffer.size() - cursor;
	}

	/**
	 * @brief	Clones this message buffer
	 */
	virtual boost::shared_ptr<CMessageBuffer> clone() const
	{
		boost::shared_ptr<CMessageBuffer> cl(new CMessageBuffer(buffer));
		cl->cursor = cursor;
		cl->setFrom(from);
		cl->setTo(to);
		cl->setType(type);
		cl->setFlowState(flowState);

		// copy properties
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
		for (pit = properties.begin(); pit != properties.end(); pit++)
			if (pit->second != NULL)
				cl->properties[pit->first] = pit->second->clone();

		return cl;
	}

	message_t& getBuffer()
	{
		return buffer;
	}

	void setBuffer(message_t& buf)
	{
		buffer = buf;
		cursor = 0;
	}

	/**
	 * @brief	Prepends a message buffer.
	 */
	inline void push_front(const CMessageBuffer& msgbuf)
	{
		assert(buffer.length() < message_max_buffers-1);
		buffer.push_front(msgbuf.buffer);
	}

	/**
	 * @brief	Prepends a message buffer.
	 */
	inline void push_front(const CMessageBuffer* msgbuf)
	{
		assert(buffer.length() < message_max_buffers-1);
		buffer.push_front(msgbuf->buffer);
	}

	/**
	 * @brief	Prepends a message buffer.
	 */
	inline void push_front(boost::shared_ptr<CMessageBuffer> msgbuf)
	{
		assert(buffer.length() < message_max_buffers-1);
		buffer.push_front(msgbuf->buffer);
	}

	/**
	 * @brief	Prepends a message buffer.
	 */
	inline void push_front(const shared_buffer_t& buffer)
	{
		assert(this->buffer.length() < message_max_buffers-1);
		this->buffer.push_front(buffer);
	}

	/**
	 * @brief	Appends a message buffer
	 */
	inline void push_back(const CMessageBuffer& msgbuf)
	{
		assert(buffer.length() < message_max_buffers-1);
		buffer.push_back(msgbuf.buffer);
	}

	/**
	 * @brief	Appends a message buffer
	 */
	inline void push_back(const CMessageBuffer* msgbuf)
	{
		assert(buffer.length() < message_max_buffers-1);
		buffer.push_back(msgbuf->buffer);
	}

	/**
	 * @brief	Appends a message buffer
	 */
	inline void push_back(boost::shared_ptr<CMessageBuffer> msgbuf)
	{
		assert(buffer.length() < message_max_buffers-1);
		buffer.push_back(msgbuf->buffer);
	}

	/**
	 * @brief	Appends a message buffer
	 */
	inline void push_back(const shared_buffer_t& buffer)
	{
		assert(this->buffer.length() < message_max_buffers-1);
		this->buffer.push_back(buffer);
	}

	inline void push_ulong(uint32_t h)
	{
		if (cursor + sizeof(h) > buffer.size()) {
			assert(false);
		}
		uint32_t n = htonl(h);
		buffer.write<uint32_t>(n, cursor);
		cursor += sizeof(n);
	}

	inline void push_ushort(ushort h)
	{
		if (cursor + sizeof(h) > buffer.size()) {
			assert(false);
		}
		ushort n = htons(h);
		buffer.write<ushort>(n, cursor);
		cursor += sizeof(n);
	}

	inline void push_uchar(unsigned char h)
	{
		if (cursor + sizeof(h) > buffer.size()) {
			assert(false);
		}
		buffer.write<unsigned char>(h, cursor++);
	}

	inline void push_string(const std::string& s)
	{
		if (cursor + s.size() > buffer.size()) {
			assert(false);
		}

		buffer.write((const boctet_t*) s.c_str(), cursor, s.size());
		cursor += s.size();
	}

	/**
	 * @brief	Copies a buffer into this buffer.
	 * 			Prepending/appending is preferred over this one.
	 */
	inline void push_buffer(const unsigned char* b, unsigned int len)
	{
		if (cursor + len > buffer.size()) {
			assert(false);
		}
		buffer.write(b, cursor, len);
		cursor += len;
	}

	inline uint32_t pop_ulong()
	{
		assert(cursor == 0);
		assert(sizeof(uint32_t) <= buffer.size());
		// should be handled more gracefully
		uint32_t n = buffer.read<uint32_t>(0);
		buffer = buffer(sizeof(n)); // chop
		return ntohl(n);
	}

	inline ushort pop_ushort()
	{
		assert(cursor == 0);
		assert(sizeof(ushort) <= buffer.size());
		// should be handled more gracefully
		ushort n = buffer.read<ushort>(0);
		buffer = buffer(sizeof(n)); // chop
		return ntohs(n);
	}

	inline unsigned char pop_uchar()
	{
		assert(cursor == 0);
		assert(sizeof(unsigned char) <= buffer.size());
		// should be handled more gracefully
		unsigned char n = buffer.read<unsigned char>(0);
		buffer = buffer(sizeof(n)); // chop
		return n;
	}

	inline std::string pop_string(unsigned int size)
	{
		if (size == 0)
			return std::string();

		assert(cursor == 0);
		assert(size <= buffer.size());
		// should be handled more gracefully
		std::string s;
		for (unsigned int i = 0; i < size; i++)
			s += buffer.read<char>(i);
		buffer = buffer(size); // chop
		return s;
	}

	/// Pops a buffer, copying its content to buf
	inline void pop_buffer(unsigned char* buf, unsigned long len)
	{
		assert(buf != NULL);
		assert(cursor == 0);
		assert(len <= buffer.size());
		// should be handled more gracefully
		buffer.read(buf, 0, len);
		buffer = buffer(len); // chop
	}

	/// Pops a buffer, creating a shared buffer object. len = 0 pops remaining buffer
	inline message_t pop_buffer(unsigned long len = 0)
	{
		assert(cursor == 0);
		assert(len <= buffer.size());
		// should be handled more gracefully
		if (len == 0)
			len = buffer.size();
		message_t buf = buffer(0, len);
		buffer = buffer(len); // chop
		return buf;
	}

	/**
	 * @brief	Serializes the header into the buffer
	 */
	inline void push_header(const IHeader& hdr)
	{
		push_front(hdr.serialize());
	}

	/**
	 * @brief	Serializes the header into the buffer
	 */
	inline void push_header(const IHeader* hdr)
	{
		push_front(hdr->serialize());
	}

	/**
	 * @brief	Serializes the header into the buffer
	 */
	inline void push_header(boost::shared_ptr<IHeader> hdr)
	{
		push_front(hdr->serialize());
	}

	/**
	 * @brief	Deserializes the header from the buffer
	 */
	template<class T>
	boost::shared_ptr<T> pop_header()
	{
		boost::shared_ptr<T> hdr(new T());
		hdr->deserialize(shared_from_this());
		return hdr;
	}

	/**
	 * @brief	Deserializes the header from the buffer
	 */
	void pop_header(IHeader& hdr)
	{
		hdr.deserialize(boost::static_pointer_cast<CMessageBuffer>(shared_from_this()));
	}

	/**
	 * @brief	Deserializes the header from the buffer, but does not remove it.
	 */
	template<class T>
	boost::shared_ptr<T> peek_header()
	{
		boost::shared_ptr<T> hdr(new T());
		// not really efficient (though buffer is NOT copied)
		boost::shared_ptr<CMessageBuffer> tmpbuf(new CMessageBuffer(buffer));
		hdr->deserialize(tmpbuf);
		return hdr;
	}

	/**
	 * @brief	Deserializes the header from the buffer, but does not remove it.
	 */
	template<class T>
	void peek_header(T& hdr)
	{
		// not really efficient (though buffer is NOT copied)
		boost::shared_ptr<CMessageBuffer> tmpbuf(new CMessageBuffer(buffer));
		hdr.deserialize(tmpbuf);
	}

	/// Pops a buffer into nirvana (useful after peeks).
	inline void remove_front(unsigned long len)
	{
		assert(cursor == 0);
		assert(len <= buffer.size());
		buffer = buffer(len); // chop
	}

	/// Pops a buffer into nirvana
	inline void remove_back(unsigned long pos)
	{
		assert(cursor == 0);
		assert(pos > 0 && pos <= buffer.size());
		buffer = buffer(0, pos); // chop
	}

	/**
	 * @brief	Sets cursor to 'pos'
	 */
	void set_cursor(std::size_t pos)
	{
		assert(pos < size());
		cursor = pos;
	}

	/**
	 * @brief	return cursor position
	 */
	std::size_t get_cursor() const
	{
		return cursor;
	}
};

/**
 * @brief	Simple class pool.
 *
 * 			For performance reasons, this class is not thread safe!
 * 			Only use within a single thread.
 *
 * 			CMessageBufferPool only manages objects, not the buffer itself.
 * 			You will always get an empty CMessageBuffer when calling get().
 *
 * 			The pool is limited to maxSize, but you will always get an object
 * 			even if the pool is exceeded. The new object simply won't be
 * 			managed by the pool.
 */
class CMessageBufferPool
{
private:
	static const std::size_t maxSize = 0;
	std::list<boost::shared_ptr<CMessageBuffer> > buffers;

public:
	/**
	 * @brief	Returns an empty CMessageBuffer object
	 */
	boost::shared_ptr<CMessageBuffer> get()
	{
		std::list<boost::shared_ptr<CMessageBuffer> >::iterator it;
		for (it = buffers.begin(); it != buffers.end(); it++) {
			boost::shared_ptr<CMessageBuffer>& p(*it);
			if (p.unique()) {
				p->reset();
				return p;
			}
		}
		boost::shared_ptr<CMessageBuffer> p(new CMessageBuffer((std::size_t) 0));
		if (buffers.size() < maxSize)
			buffers.push_back(p);
		return p;
	}

};

/**
 * @brief	Pool for fixed-size shared_buffer_t buffers.
 *
 * 			For performance reasons, this class is not thread safe!
 * 			Only use within a single thread.
 *
 * 			The pool is limited to maxSize, but you will always get an object
 * 			even if the pool is exceeded. The new object simply won't be
 * 			managed by the pool.
 *
 * 			Note, the shared_buffer_t::use_count() MAY be increased by one if
 * 			managed by this pool.
 */
class CSharedBufferPool
{
private:
	static const std::size_t maxSize = 64;
	std::list<shared_buffer_t> buffers;
	std::size_t bufferSize;

public:
	CSharedBufferPool(std::size_t bufferSize) :
			bufferSize(bufferSize)
	{
	}

	virtual ~CSharedBufferPool()
	{
	}

	/**
	 * @brief	Returns a shared buffer with size bufferSize
	 */
	shared_buffer_t get()
	{
		std::list<shared_buffer_t>::iterator it;
		for (it = buffers.begin(); it != buffers.end(); it++) {
			shared_buffer_t& p(*it);
			if (p.use_count() == 1) {
				return p;
			}
		}
		shared_buffer_t p(bufferSize);
		if (buffers.size() < maxSize)
			buffers.push_back(p);
		return p;
	}

};

#endif /* MESSAGEBUFFERS_H_ */
