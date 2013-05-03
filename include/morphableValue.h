/** @file
 * morphableValue.h
 *
 * @brief Classes for a morphable value, e.g. a property, config parameter etc.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Jun 30, 2009
 *      Author: denis
 */

#ifndef MORPHABLEVALUE_H_
#define MORPHABLEVALUE_H_

#include <string>
#include <sstream>
#include <list>
#include <exception>

#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <string.h>

#include "debug.h"

/**
 * @brief Base class for morphable values, e.g. properties, config parameters etc.
 *
 * This is not a variant ;)
 */
class CMorphableValue : public boost::enable_shared_from_this<CMorphableValue>
{
public:
	enum ValueType { vt_none, vt_bool, vt_int, vt_double, vt_string, vt_buffer, vt_any, vt_custom };
	enum ContainerType { ct_simple, ct_range, ct_list };

	template <typename T>
	class Range
	{
	public:
		T lowerBound;
		T upperBound;

		Range() {};

		Range(T lowerB, T upperB):
			lowerBound(lowerB),
			upperBound(upperB)
		{};

	};

	class EValueTypeMismatch : public std::exception {};
	class EContainerTypeMismatch : public std::exception {};

protected:
	ValueType valueType;
	ContainerType containerType;

public:
	CMorphableValue(ValueType vt = vt_none, ContainerType ct = ct_simple):
		valueType(vt), containerType(ct)
	{};
	virtual ~CMorphableValue() {};

	inline ValueType getValueType() const {return valueType;};
	inline ContainerType getContainerType() const {return containerType;};

	virtual std::string toStr() const = 0;

	template<class T>
	boost::shared_ptr<T> cast() throw (EValueTypeMismatch) {
		boost::shared_ptr<CMorphableValue> self = shared_from_this();
		boost::shared_ptr<T> r = boost::dynamic_pointer_cast<T, CMorphableValue>(self);
		if (r.get() == NULL) throw EValueTypeMismatch(); // not very elegant...
		return r;
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const = 0;
};

/**
 * @brief	Internal use only; container class for a value range
 */
template <class T>
class CValueRange : public CMorphableValue
{
protected:
	Range<T> range_v;

public:
	CValueRange(ValueType vt = vt_none):
		CMorphableValue(vt, ct_range)
	{};

	inline Range<T>& range() throw (EValueTypeMismatch, EContainerTypeMismatch)
	{
		if (containerType != ct_range) throw EContainerTypeMismatch();
		return range_v;
	};
};

/**
 * @brief 	Internal use only; container class for a value range
 */
template <class T>
class CValueList : public CMorphableValue
{
protected:
	std::list<T> list_v;

public:
	CValueList(ValueType vt = vt_none):
		CMorphableValue(vt, ct_list)
	{};

	inline std::list<T>& list() throw (EValueTypeMismatch, EContainerTypeMismatch)
	{
		if (containerType != ct_list) throw EContainerTypeMismatch();
		return list_v;
	};
};

/* Simple values *************************************************************/

/**
 * @brief	Facade for a variant type based on boost::any
 */
class CVariantValue : public CMorphableValue
{
private:
	boost::any _v;

public:
	CVariantValue(const boost::any& v = boost::any()):
		CMorphableValue(vt_any), _v(v)
	{};

	inline boost::any value() const {return _v;}

	virtual std::string toStr() const
	{
		const std::type_info& ti = _v.type();

		if (ti == typeid(std::string))
			return boost::lexical_cast<std::string>(boost::any_cast<std::string>(_v));

		else if (ti == typeid(int))
			return boost::lexical_cast<std::string>(boost::any_cast<int>(_v));

		else if (ti == typeid(short))
			return boost::lexical_cast<std::string>(boost::any_cast<short>(_v));

		else if (ti == typeid(bool))
			return boost::lexical_cast<std::string>(boost::any_cast<bool>(_v));

		else
			return (FMT("(CVariantValue::toStr(): type %1% not supported)") % ti.name()).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CVariantValue(_v));
		return mv;
	}

	/**
	 * @brief	Wrapper for boost::any_cast<>()
	 */
	template <typename T>
	T cast()
	{
		return boost::any_cast<T>(_v);
	}
};

/**
 * @brief Facade for bool values
 */
class CBoolValue : public CMorphableValue
{
private:
	bool _v;

public:
	CBoolValue(const bool v = false):
		CMorphableValue(vt_bool), _v(v)
	{};

	inline bool value() const {return _v;}
	inline void set(bool v) {_v = v;}

	virtual std::string toStr() const
	{
		return _v ? "true" : "false";
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CBoolValue(_v));
		return mv;
	}
};

/**
 * @brief Facade for int values
 */
class CIntValue : public CMorphableValue
{
private:
	int _v;

public:
	CIntValue(const int v = 0):
		CMorphableValue(vt_int), _v(v)
	{};

	inline int value() const {return _v;}
	inline void set(int v) {_v = v;}

	virtual std::string toStr() const
	{
		return (FMT("%1%") % _v).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CIntValue(_v));
		return mv;
	}
};

/**
 * @brief Facade for pointer values
 */
template<class T>
class CPointerValue : public CMorphableValue
{
private:
	T* _v;

public:
	CPointerValue(T* v = NULL):
		CMorphableValue(vt_int), _v(v)
	{};

	inline T* value() const {return _v;}

	virtual std::string toStr() const
	{
		return (FMT("%1$p") % _v).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CPointerValue<T>(_v));
		return mv;
	}
};

/**
 * @brief Facade for shared pointer values
 */
template<class T>
class CSharedPtrValue : public CMorphableValue
{
private:
	boost::shared_ptr<T> _v;

public:
	CSharedPtrValue(boost::shared_ptr<T> v = boost::shared_ptr<T>()):
		CMorphableValue(vt_int), _v(v)
	{};

	inline boost::shared_ptr<T> value() const {return _v;}

	virtual std::string toStr() const
	{
		return (FMT("%1$p") % _v.get()).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CSharedPtrValue<T>(_v));
		return mv;
	}
};

/**
 * @brief Facade for int values
 */
class CDoubleValue : public CMorphableValue
{
private:
	double _v;

public:
	CDoubleValue(const double v = 0):
		CMorphableValue(vt_double), _v(v)
	{};

	inline double value() const {return _v;}
	inline void set(double v) {_v = v;}

	virtual std::string toStr() const
	{
		return (FMT("%1%") % _v).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CDoubleValue(_v));
		return mv;
	}
};

/**
 * @brief Facade for string values
 */
class CStringValue : public CMorphableValue
{
private:
	std::string _v;

public:
	CStringValue(const std::string v = 0):
		CMorphableValue(vt_string), _v(v)
	{};

	CStringValue (const CStringValue & rhs):
		CMorphableValue(vt_string)
	{
		_v = rhs._v;
	}

	inline std::string value() const {return _v;}
	inline void set(std::string v) {_v = v;}

	virtual std::string toStr() const
	{
		return _v;
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CStringValue(_v));
		return mv;
	}
};

/**
 * @brief Facade for buffer values
 */
class CBufferValue : public CMorphableValue
{
private:
	char* _v;
	std::size_t _size;

public:
	/**
	 * @brief	Constructor.
	 *
	 * 			If v is NULL, a new buffer is allocated. The buffer will be
	 * 			always freed when this class is destroyed. The size of the
	 * 			buffer is fixed and cannot be changed.
	 */
	CBufferValue(std::size_t size = 0, char* v = NULL):
		CMorphableValue(vt_buffer), _v(v), _size(size)
	{
		assert(size >= 0);

		if (v == NULL && size > 0)
			_v = new char[size];
	};

	virtual ~CBufferValue()
	{
		if (_v != NULL)
			delete[] _v;
	};

	inline char* value() const {return _v;}
	inline std::size_t size() const {return _size;}

	virtual std::string toStr() const
	{
		return (FMT("Buffer(%1$4x, %2%)") % _v % _size).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		char* b = new char[_size];
		memcpy(b, _v, _size);
		boost::shared_ptr<CMorphableValue> mv(new CBufferValue(_size, b));
		return mv;
	}
};

/* Value ranges **************************************************************/

/**
 * @brief Facade for int ranges
 */
class CIntRange : public CValueRange<int>
{
public:
	CIntRange(int lower = 0, int upper = 0):
		CValueRange<int>(vt_int)
	{
		range_v.lowerBound = lower;
		range_v.upperBound = upper;
	};

	virtual std::string toStr() const
	{
		return (FMT("(%1%, %2%)") % range_v.lowerBound % range_v.upperBound).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CIntRange(range_v.lowerBound, range_v.upperBound));
		return mv;
	}
};

/**
 * @brief Facade for double ranges
 */
class CDoubleRange : public CValueRange<double>
{
public:
	CDoubleRange(double lower = 0, double upper = 0):
		CValueRange<double>(vt_double)
	{
		range_v.lowerBound = lower;
		range_v.upperBound = upper;
	};

	virtual std::string toStr() const
	{
		return (FMT("(%1%, %2%)") % range_v.lowerBound % range_v.upperBound).str();
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		boost::shared_ptr<CMorphableValue> mv(new CDoubleRange(range_v.lowerBound, range_v.upperBound));
		return mv;
	}
};

/* Value lists ***************************************************************/

/**
 * @brief Facade for an int list
 */
class CIntList : public CValueList<int>
{
public:
	CIntList():
		CValueList<int>(vt_int)
	{};

	virtual std::string toStr() const
	{
		std::string s = "[";
		std::list<int>::const_iterator it;
		for (it = list_v.begin(); it != list_v.end(); /* nothing */) {
			s += (FMT("%1%") % *it).str();
			it++;
			if (it != list_v.end())
				s += ", ";
		}
		return s += "]";
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		// TODO
		assert(false);
		//return (CMorphableValue*) new CIntList(*this);
		boost::shared_ptr<CMorphableValue> mv;
		return mv;
	}
};

/**
 * @brief Facade for an int list
 */
class CDoubleList : public CValueList<double>
{
public:
	CDoubleList():
		CValueList<double>(vt_double)
	{};

	virtual std::string toStr() const
	{
		std::string s = "[";
		std::list<double>::const_iterator it;
		for (it = list_v.begin(); it != list_v.end(); /* nothing */) {
			s += (FMT("%1%") % *it).str();
			it++;
			if (it != list_v.end())
				s += ", ";
		}
		return s += "]";
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		// TODO
		assert(false);
		//return (CMorphableValue*) new CDoubleList(*this);
		boost::shared_ptr<CMorphableValue> mv;
		return mv;
	}
};

/**
 * @brief Facade for a string list
 */
class CStringList : public CValueList<std::string>
{
public:
	CStringList():
		CValueList<std::string>(vt_string)
	{};

	virtual std::string toStr() const
	{
		std::string s = "[";
		std::list<std::string>::const_iterator it;
		for (it = list_v.begin(); it != list_v.end(); /* nothing */) {
			s += *it;
			it++;
			if (it != list_v.end())
				s += ", ";
		}
		return s += "]";
	}

	virtual boost::shared_ptr<CMorphableValue> clone() const
	{
		// TODO
		assert(false);
		//return (CMorphableValue*) new CStringList(*this);
		boost::shared_ptr<CMorphableValue> mv;
		return mv;
	}
};

#endif /* MORPHABLEVALUE_H_ */
