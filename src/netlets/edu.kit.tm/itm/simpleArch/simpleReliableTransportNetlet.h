/** @file
 * simpleReliableTransportNetlet.h
 *
 * @brief Simple example of a composed reliable transport Netlet
 *
 * (c) 2008-2012 Institut fuer Telematik, KIT, Germany
 *
 *  Created on: Aug 14, 2009
 *      Author: denis
 */

#ifndef SIMPLERELIABLETRANSPORTNETLET_H_
#define SIMPLERELIABLETRANSPORTNETLET_H_

#include "simpleComposedNetlet.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

#include <string>

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

class CSimpleReliableTransportNetletMetaData;

/**
 * @brief	A Netlet taking its actual building block configuration from
 * 			a generated source file.
 *
 * @TODO	Per architecture, a single template for a composable Netlet would
 * 			be sufficient.
 */
class CSimpleReliableTransportNetlet : public CSimpleComposedNetlet
{
private:
	CSimpleReliableTransportNetletMetaData* metaData;

public:
	CSimpleReliableTransportNetlet(CSimpleReliableTransportNetletMetaData* metaData, CNena *nodeA, IMessageScheduler *sched);
	virtual ~CSimpleReliableTransportNetlet();

	// from INetlet

	/**
	 * @brief	Returns the Netlet's meta data
	 */
	virtual INetletMetaData* getMetaData() const;
};

/**
 * @brief Simple composed Netlet meta data
 *
 * The Netlet meta data class has two purposes: On one hand, it describes
 * the Netlet's properties, capabilities, etc., on the other hand, it provides
 * a factory functions that will be used by the node architecture daemon to
 * instantiate new Netlets of this type.
 */
class CSimpleReliableTransportNetletMetaData : public INetletMetaData
{
private:
	std::string archName;
	std::string netletId;
	std::map<std::string, std::string> properties;

public:
	CSimpleReliableTransportNetletMetaData(const std::string& archName, const std::string& netletName = std::string());
	virtual ~CSimpleReliableTransportNetletMetaData();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;
	virtual int canHandle(const std::string& uri, std::string& req) const;
	virtual std::string getProperty(const std::string& prop) const;

	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched); // factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLERELIABLETRANSPORTNETLET_H_ */
