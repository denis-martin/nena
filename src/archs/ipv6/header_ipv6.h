
#include <arpa/inet.h>
#include <netinet/ip6.h>

namespace ipv6 {

/* ========================================================================= */

/**
 * @brief Minimum header containing a hash of the application's service ID
 */
class IPv6_Header: public IHeader
{
public:
	struct ip6_hdr ip6hdr;

	IPv6_Header()
	{
		memset(&ip6hdr, 0, sizeof(struct ip6_hdr));
		ip6hdr.ip6_flow = htonl(6 << 28);
		ip6hdr.ip6_hops = 255;
	};

	virtual ~IPv6_Header() {};

	inline uint8_t getVersion() { return (uint8_t) ((ntohl(ip6hdr.ip6_flow) & 0xF0000000) >> 28); }
	inline uint8_t getTrafficClass() { return (uint8_t) ((ntohl(ip6hdr.ip6_flow) & 0x0FF00000) >> 20); }
	inline uint32_t getFlowLabel() { return (ntohl(ip6hdr.ip6_flow) & 0x000FFFFF); }

	inline uint32_t getPayloadLength() { return ntohs(ip6hdr.ip6_plen); }
	inline uint8_t getNextHeader() { return (uint8_t) ip6hdr.ip6_nxt; }
	inline uint8_t getHopLimit() { return (uint8_t) ip6hdr.ip6_hops; }

	inline std::string getSrcAddrStr() {
		char s[INET6_ADDRSTRLEN];
		return std::string(inet_ntop(AF_INET6, (const void*) &ip6hdr.ip6_src, s, INET6_ADDRSTRLEN));
	}

	inline std::string getDstAddrStr() {
		char s[INET6_ADDRSTRLEN];
		return std::string(inet_ntop(AF_INET6, (const void*) &ip6hdr.ip6_dst, s, INET6_ADDRSTRLEN));
	}

	/**
	 * @brief Serialize all relevant data into a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual boost::shared_ptr<CMessageBuffer> serialize() const
	{
		std::size_t s = sizeof(struct ip6_hdr);
		boost::shared_ptr<CMessageBuffer> mbuf(new CMessageBuffer(s));
		mbuf->push_buffer((const unsigned char *) &ip6hdr, sizeof(struct ip6_hdr));
		return mbuf;
	}

	/**
	 * @brief De-serialize all relevant data from a byte buffer. Remember to do Little/Big Endian conversion.
	 */
	virtual void deserialize(boost::shared_ptr<CMessageBuffer> mbuf)
	{
		assert(mbuf.get() != NULL);
		assert(mbuf->getBuffer().length() == 1);

		mbuf->pop_buffer((uint8_t*) &ip6hdr, sizeof(ip6_hdr));
	}

};

} // namespace ipv6
