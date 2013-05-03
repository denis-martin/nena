/** @file
 * address.h
 *
 * @brief Classes for IPv4 addressing
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Sep 1, 2009
 *      Author: denis
 */

#ifndef ARCHDEP_IPV4_ADDRESS_H_
#define ARCHDEP_IPV4_ADDRESS_H_

#include "morphableValue.h"

#include <boost/spirit/include/classic_core.hpp>

namespace ipv4 {

/**
 * @brief CMorphableValue facade for an IPv4 socket address
 */
class CLocatorValue : public CMorphableValue
{
private:
	unsigned long _addr;
	unsigned short _port;
	bool _isValid;

public:
	CLocatorValue(unsigned long addr = 0, unsigned short port = 0):
		CMorphableValue(vt_custom), _addr(addr), _port(port)
	{
		_isValid = (addr != 0) || (port != 0);
	};

	CLocatorValue(const std::string& addr):
		CMorphableValue(vt_custom)
	{
		_addr = 0;
		_port = 0;
		_isValid = false;

		setAddr(addr);
	};

	CLocatorValue (const CLocatorValue & rhs):
		CMorphableValue(vt_custom)
	{
		_addr = rhs._addr;
		_port = rhs._port;
		_isValid = rhs._isValid;
	}

	CLocatorValue & operator= (const CLocatorValue & rhs)
	{
		if (this != &rhs)
		{
			_addr = rhs._addr;
			_port = rhs._port;
			_isValid = rhs._isValid;
		}

		return *this;
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CLocatorValue(*this));
		return mv;
	}

	/**
	 * @brief	Returns the IPv4 address in host format
	 */
	inline unsigned long getAddr() const {return _addr;}

	/**
	 * @brief	Sets the IPv4 address in host format
	 */
	inline void setAddr(unsigned long addr)
	{
		_addr = addr;
		if (_addr != 0 && _port != 0)
			_isValid = true;
	}

	/**
	 * @brief	Returns the port number in host format
	 */
	inline unsigned short getPort() const {return _port;}

	/**
	 * @brief	Sets the port number in host format
	 */
	inline void setPort(unsigned short port)
	{
		_port = port;
		if (_addr != 0 && _port != 0)
			_isValid = true;
	}

	/**
	 * @brief	Returns true if the value is valid.
	 */
	inline bool isValid() const {return _isValid;}

	/**
	 * @brief	Decodes the given string into IPv4 address and port.
	 *
	 * @return	True if conversion was successful, false otherwise.
	 */
	inline bool setAddr(const std::string& addr)
	{
		unsigned int a, b, c, d, port;
		boost::spirit::classic::rule<> rule =
			boost::spirit::classic::int_p[boost::spirit::classic::assign_a(a)] >> boost::spirit::classic::str_p(".") >>
			boost::spirit::classic::int_p[boost::spirit::classic::assign_a(b)] >> boost::spirit::classic::str_p(".") >>
			boost::spirit::classic::int_p[boost::spirit::classic::assign_a(c)] >> boost::spirit::classic::str_p(".") >>
			boost::spirit::classic::int_p[boost::spirit::classic::assign_a(d)] >> boost::spirit::classic::str_p(":") >>
			boost::spirit::classic::int_p[boost::spirit::classic::assign_a(port)];

		if (boost::spirit::classic::parse(addr.c_str(), rule, boost::spirit::classic::space_p).full &&
			(a >= 0 && a < 256) &&
			(b >= 0 && b < 256) &&
			(c >= 0 && c < 256) &&
			(d >= 0 && d < 256) &&
			(port >= 0 && port < 0xffff))
		{
			_addr = a << 24 | b << 16 | c << 8 | d;
			_port = port;
			_isValid = true;

		} else {
			_isValid = false;

		}

		return _isValid;
	};

	inline std::string addrToStr()
	{
		return boost::str(FMT("%1%.%2%.%3%.%4%") %
			(_addr >> 24) % ((_addr >> 16) & 0xff) % ((_addr >> 8) & 0xff) % (_addr & 0xff));
	}

	virtual std::string toStr() const
	{
		return boost::str(FMT("%1%.%2%.%3%.%4%:%5%") %
			(_addr >> 24) % ((_addr >> 16) & 0xff) % ((_addr >> 8) & 0xff) % (_addr & 0xff) %
			_port);
	}

	bool operator== (const CLocatorValue & rhs) const
	{
		return ((_addr == rhs.getAddr()) && (_port == rhs.getPort()));
	}

	bool operator!= (const CLocatorValue & rhs) const
	{
		return ((_addr != rhs.getAddr()) || (_port != rhs.getPort()));
	}

	bool operator< (const CLocatorValue & rhs) const
	{
		return (_addr < rhs.getAddr()) || ((_addr == rhs.getAddr() && _port < rhs.getPort()));
	}

	bool operator> (const CLocatorValue & rhs) const
	{
		return (_addr > rhs.getAddr()) || ((_addr == rhs.getAddr() && _port > rhs.getPort()));
	}

	bool operator<= (const CLocatorValue & rhs) const
	{
		return (_addr < rhs.getAddr()) || ((_addr == rhs.getAddr() && _port <= rhs.getPort()));
	}

	bool operator>= (const CLocatorValue & rhs) const
	{
		return (_addr > rhs.getAddr()) || ((_addr == rhs.getAddr() && _port >= rhs.getPort()));
	}
};

/**
 * @brief Facade for a locator list
 */
class CLocatorList : public CValueList<boost::shared_ptr<CLocatorValue> >
{
public:
	CLocatorList():
		CValueList<boost::shared_ptr<CLocatorValue> >(vt_custom)
	{};

	virtual std::string toStr() const
	{
		std::string s = "[";
		std::list<boost::shared_ptr<CLocatorValue> >::const_iterator it;
		for (it = list_v.begin(); it != list_v.end(); /* nothing */) {
			s += (*it)->toStr();
			it++;
			if (it != list_v.end())
				s += ", ";
		}
		return s += "]";
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CLocatorList(*this));
		return mv;
	}
};

} // namespace ipv4

#endif // ARCHDEP_IPV4_ADDRESS_H_
