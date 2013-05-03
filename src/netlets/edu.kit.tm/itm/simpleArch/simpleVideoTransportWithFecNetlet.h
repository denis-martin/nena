/** @file
 * simpleVideoTransportWithFecNetlet.h
 *
 * @brief Simple example of a composed multimedia Netlet
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 14, 2009
 *      Author: denis
 */

#ifndef SIMPLEVIDEOTRANSPORTWITHFECNETLET_H_
#define SIMPLEVIDEOTRANSPORTWITHFECNETLET_H_

#include "simpleComposedNetlet.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

const std::string& SIMPLE_VIDEOTRANSPORTWITHFEC_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/SimpleVideoTransportWithFecNetlet";

/**
 * @brief	A Netlet taking its actual building block configuration from
 * 			a generated source file.
 *
 * @TODO	Per architecture, a single template for a composable Netlet would
 * 			be sufficient.
 */
class CSimpleVideoTransportWithFecNetlet : public CSimpleComposedNetlet
{
public:
	CSimpleVideoTransportWithFecNetlet(CNena *nodeA, IMessageScheduler *sched);
	virtual ~CSimpleVideoTransportWithFecNetlet();

	// from INetlet

	/**
	 * @brief	Returns the Netlet's meta data
	 */
	virtual INetletMetaData* getMetaData() const;

	virtual const std::string & getId () const { return getMetaData()->getId (); }

	// own methods

	virtual void setFecLevels(int levels);
};

/**
 * @brief Simple composed Netlet meta data
 *
 * The Netlet meta data class has two purposes: On one hand, it describes
 * the Netlet's properties, capabilities, etc., on the other hand, it provides
 * a factory functions that will be used by the node architecture daemon to
 * instantiate new Netlets of this type.
 */
class CSimpleVideoTransportWithFecNetletMetaData : public INetletMetaData
{
public:
	CSimpleVideoTransportWithFecNetletMetaData();
	virtual ~CSimpleVideoTransportWithFecNetletMetaData();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;
	virtual int canHandle(const std::string& uri, std::string& req) const;

	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched); // factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLEVIDEOTRANSPORTWITHFECNETLET_H_ */
