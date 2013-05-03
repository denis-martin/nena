/** @file
 * netAdapt.h
 *
 * @brief Generic interface for network accesses.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#ifndef NETADAPT_H_
#define NETADAPT_H_

#include "messages.h"
#include "morphableValue.h"
#include <exceptions.h>

#include <string>
#include <map>

class CNena;

/**
 * @brief Generic Network Adaptor Interface
 */
class INetAdapt : public IMessageProcessor
{
public:
	enum PropertyId {
		p_min				= 0,

		// generic properties
		p_generic_min,
		p_name,							// string
		p_archid,						// network architecture ID (string)
		p_addr,							// architecture specific address (string)
		p_up,							// bool
		p_linkencap,					// string: Ethernet, 802.11, ZigBee, Bluetooth, WiMax, ...
		p_bandwidth,					// Mbit/s
		p_freeBandwidth,				// Mbit/s
		p_broadcast,					// bool
		p_rx_packets,					// unsigned int
		p_rx_errors,					// unsigned int
		p_rx_dropped,					// unsigned int
		p_rx_rate,						// unsigned int (bytes / second)
		p_tx_packets,					// unsigned int
		p_tx_errors,					// unsigned int
		p_tx_dropped,					// unsigned int
		p_tx_rate,						// unsigned int (bytes / second)
		p_mtu,							// unsigned int
		p_duplex,						// bool
		p_virtualized,					// bool (false means: PHY or unknown)
		p_generic_max,

		// properties related to PHY
		p_phy_min			= 1000,
		p_phy_max,

		// properties related to virtual networks
		p_virt_min			= 2000,
		p_virt_max,

		// properties related to MAC layer
		p_mac_min			= 3000,
		p_mac_max,

		// properties related to power management
		p_pow_min			= 4000,
		p_pow_sleepmodes,				// ???
		p_pow_powerctrl,				// ???
		p_pow_max,

		// properties related to QoS - maybe differentiate between RX/TX?
		p_qos_min			= 5000,
		p_qos_bandwidth,				// range
		p_qos_avgdelay,					// double
		p_qos_jitter,					// double
		p_qos_pktloss_rate,				// double
		p_qos_biterror_rate,			// double
		p_qos_max,

		p_max

	};

	class EPropertyNotDefined : public std::exception {};

protected:
	CNena *nena;
	std::string arch; ///< URI of architecture
	std::map<PropertyId, boost::shared_ptr<CMorphableValue> > properties;

public:
	NENA_EXCEPTION(EConfig);

	INetAdapt(CNena *nodeA, IMessageScheduler *sched, const std::string& arch, const std::string& uri) :
		IMessageProcessor(sched), nena(nodeA), arch(arch)
	{
		className += "::INetAdapt";
		setId(uri);
	};

	virtual ~INetAdapt()
	{};

	/**
	 * @brief	Check whether network adaptor is ready for sending/receiving data
	 */
	virtual bool isReady() const = 0;

	/**
	 * @brief	Return URI of network architecture
	 */
	virtual const std::string& getArchId() const { return arch; };

	/**
	 * @brief	Set URI of network architecture
	 */
	virtual void setArchId(const std::string& arch) { this->arch = arch; };

	/**
	 * @brief	Safely return a property.
	 *
	 * 			As template parameter, use the effective value type, e.g.
	 * 			int, string, etc. Use CMorphableValue* getProperty() if you
	 * 			do not know the effective value type.
	 *
	 * @throw	EPropertyNotDefined if the property is not set.
	 * @throw	CMorphableValue::EValueTypeMismatch if the given value type
	 * 			does not match the effective value type.
	 *
	 * @return	A copy of the property value.
	 */
	template <typename T>
	boost::shared_ptr<T> getProperty(PropertyId pid) const throw (EPropertyNotDefined, CMorphableValue::EValueTypeMismatch)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::const_iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end())
			return pit->second->cast<T>(); // return a copy
		else
			throw EPropertyNotDefined();
	};

	/**
	 * @brief	Safely return a property.
	 *
	 * @throw	EPropertyNotDefined if the property is not set.
	 *
	 * @return	The property's value.
	 */
	const boost::shared_ptr<CMorphableValue> getProperty(PropertyId pid) throw (EPropertyNotDefined)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end())
			return pit->second; // return a copy
		else
			throw EPropertyNotDefined();
	};

	/**
	 * @brief	Convenience function: Checks if the given property exists
	 * 			and returns its value in the second parameter
	 *
	 * @param	pid		Property ID
	 * @param	mv		Reference to CMorphableValue pointer variable where
	 * 					the address of the result is stored in case the
	 * 					property exists. The value is undefined if the property
	 * 					does not exist.
	 *
	 * @return	True if the requested property exists, false otherwise
	 */
	bool hasProperty(PropertyId pid, boost::shared_ptr<CMorphableValue>& mv)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			mv = pit->second; // return a pointer
			return true;

		} else {
			mv.reset();
			return false;

		}
	};

	/**
	 * @brief	Safely set a property, erasing the old one if necessary.
	 */
	inline void setProperty(PropertyId pid, boost::shared_ptr<CMorphableValue> val)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			if (val == NULL) {
				properties.erase(pit);
				return;
			}

		}

		if (val != NULL)
			properties[pid] = val;
	}

	/**
	 * @brief	Safely set a property, erasing the old one if necessary.
	 * 			Convenience method. Ownership of val is transfered to a shared_ptr.
	 */
	inline void setProperty(const PropertyId pid, CMorphableValue* val)
	{
		std::map<PropertyId, boost::shared_ptr<CMorphableValue> >::iterator pit;
		pit = properties.find(pid);
		if (pit != properties.end()) {
			if (val == 0) {
				properties.erase(pit);
				return;
			}

		}

		if (val != 0)
			properties[pid] = boost::shared_ptr<CMorphableValue>(val);
	}

};

#endif /* NETADAPT_H_ */
