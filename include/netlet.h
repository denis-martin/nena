/** @file
 * netlet.h
 *
 * @brief Generic interface for all Netlets and Netlet meta data.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#ifndef NETLET_H_
#define NETLET_H_

#include "messages.h"
#include "debug.h"

#include <map>
#include <string>

class CNena;
class INetlet;

typedef std::string	NetletName;

/**
 * @brief Netlet meta data.
 *
 * The Netlet meta data class has two purposes: On one hand, it describes
 * the Netlet's properties, capabilities, etc., on the other hand, it provides
 * a factory functions that will be used by the node architecture daemon to
 * instantiate new Netlets of this type.
 */
class INetletMetaData
{
public:
	enum PropertyId {
		p_none = 0,
		p_reliabilityScore,	///< video transport reliability score (temporarily added)
		p_userBase = 1000
	};

	/**
	 * @brief	Thrown if the requested property is not set.
	 */
	class EPropertyNotDefined : public std::exception {};

protected:
	/**
	 * @brief	Netlet properties.
	 */
//	std::map<PropertyId, boost::shared_ptr<CMorphableValue> > properties;

public:
	INetletMetaData() {};
	virtual ~INetletMetaData()
	{
//		properties.clear();
	};

	/**
	 * @brief 	Must return name of architecture the Netlet belongs to.
	 */
	virtual std::string getArchName() const = 0;

	/**
	 * @brief 	Must return a unique name of the Netlet type.
	 */
	virtual const std::string& getId() const = 0;

	/**
	 * @brief	Returns true if the Netlet does not offer any transport service
	 * 			for applications, false otherwise.
	 */
	virtual bool isControlNetlet() const = 0;

	/**
	 * @brief	Returns confidence value for the given name (uri) and the given
	 * 			requirements (req). A value of 0 means, that the Netlet cannot
	 * 			handle the request at all.
	 */
	virtual int canHandle(const std::string& uri, std::string& req) const
	{
		return 0;
	}

	/**
	 * @brief 	Netlet factory function.
	 *
	 * Creates an instance of the Netlet type and returns a pointer to it.
	 */
	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched) = 0; // factory function

	/**
	 * @brief	Return static Netlet property
	 */
	virtual std::string getProperty(const std::string& prop) const
	{
		return std::string();
	}

//	/**
//	 * @brief	Safely set a property, erasing the old one if necessary.
//	 */
//	inline void setProperty(PropertyId pid, boost::shared_ptr<CMorphableValue> val)
//	{
//		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
//		pit = properties.find(pid);
//		if (pit != properties.end()) {
//			if (val == NULL) {
//				properties.erase(pit);
//				return;
//			}
//
//		}
//
//		if (val != NULL)
//			properties[pid] = val;
//	}
//
//	/**
//	 * @brief	Safely return a property.
//	 *
//	 * 			As template parameter, use the effective value type, e.g.
//	 * 			int, string, etc. Use CMorphableValue* getProperty() if you
//	 * 			do not know the effective value type.
//	 *
//	 * @throw	EPropertyNotDefined if the property is not set.
//	 * @throw	CMorphableValue::EValueTypeMismatch if the given value type
//	 * 			does not match the effective value type.
//	 *
//	 * @return	A copy of the property value.
//	 */
//	template <typename T>
//	boost::shared_ptr<T> getProperty(PropertyId pid) const throw (EPropertyNotDefined, CMorphableValue::EValueTypeMismatch)
//	{
//		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
//		pit = properties.find(pid);
//		if (pit != properties.end())
//			return pit->second->cast<T>();
//		else
//			throw EPropertyNotDefined();
//	};
//
//	/**
//	 * @brief	Safely return a property.
//	 *
//	 * @throw	EPropertyNotDefined if the property is not set.
//	 *
//	 * @return	The property's value.
//	 */
//	const boost::shared_ptr<CMorphableValue> getProperty(PropertyId pid) const throw (EPropertyNotDefined)
//	{
//		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
//		pit = properties.find(pid);
//		if (pit != properties.end())
//			return pit->second;
//		else
//			throw EPropertyNotDefined();
//	};
//
//	/**
//	 * @brief	Convenience function: Checks if the given property exists
//	 * 			and returns its value in the second parameter
//	 *
//	 * @param	pid		Property ID
//	 * @param	mv		Reference to CMorphableValue pointer variable where
//	 * 					the address of the result is stored in case the
//	 * 					property exists. The value is undefined if the property
//	 * 					does not exist.
//	 *
//	 * @return	True if the requested property exists, false otherwise
//	 */
//	bool hasProperty(PropertyId pid, boost::shared_ptr<CMorphableValue>& mv) const
//	{
//		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
//		pit = properties.find(pid);
//		if (pit != properties.end()) {
//			mv = pit->second;
//			return true;
//
//		} else {
//			mv.reset();
//			return false;
//
//		}
//	};

};

/**
 * @brief Generic Netlet Interface.
 *
 * Must be implemented by all Netlets.
 */
class INetlet : public IMessageProcessor
{
protected:
	CNena *nena;

public:
	/**
	 * @brief Constructor.
	 */
	INetlet(CNena *nena, IMessageScheduler *sched) :
		IMessageProcessor(sched), nena(nena)
	{
		className += "::INetlet";
	};

	/**
	 * @brief Destructor.
	 */
	virtual ~INetlet()
	{
	};

	/**
	 * @brief	Returns the Netlet's meta data
	 */
	virtual INetletMetaData* getMetaData() const = 0;
};

/// Type for global collection of Netlet factories.
typedef std::map<std::string, std::map<std::string, INetletMetaData*> > NetletFactories;

/// Global collection of available Netlet factories (allocated in Node Arch Daemon).
extern NetletFactories netletFactories;

#endif /* NETLET_H_ */
