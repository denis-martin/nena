/** @file
 * simpleVideoTransportNetlet.h
 *
 * @brief Simple example of a composed multimedia Netlet
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 14, 2009
 *      Author: denis
 */

#ifndef SIMPLEVIDEOTRANSPORTNETLET_H_
#define SIMPLEVIDEOTRANSPORTNETLET_H_

#include "simpleComposedNetlet.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

const std::string& SIMPLE_VIDEOTRANSPORT_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/SimpleVideoTransportNetlet";

/**
 * @brief	A Netlet taking its actual building block configuration from
 * 			a generated source file.
 *
 * @TODO	Per architecture, a single template for a composable Netlet would
 * 			be sufficient.
 */
class CSimpleVideoTransportNetlet : public CSimpleComposedNetlet
{
public:
	CSimpleVideoTransportNetlet(CNena *nodeA, IMessageScheduler *sched);
	virtual ~CSimpleVideoTransportNetlet();

	// from INetlet

	/**
	 * @brief	Returns the Netlet's meta data
	 */
	virtual INetletMetaData* getMetaData() const;

	virtual const std::string & getId () const { return getMetaData()->getId (); }
};

/**
 * @brief Simple composed Netlet meta data
 *
 * The Netlet meta data class has two purposes: On one hand, it describes
 * the Netlet's properties, capabilities, etc., on the other hand, it provides
 * a factory functions that will be used by the node architecture daemon to
 * instantiate new Netlets of this type.
 */
class CSimpleVideoTransportNetletMetaData : public INetletMetaData
{
public:
	CSimpleVideoTransportNetletMetaData();
	virtual ~CSimpleVideoTransportNetletMetaData();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;
	virtual int canHandle(const std::string& uri, std::string& req) const;

	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched); // factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLEVIDEOTRANSPORTNETLET_H_ */
