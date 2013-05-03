/** @file
 * cryptTestNetlet.h
 *
 * @brief Netlet which provides AES encryption for testing purposes
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: April 18, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _CRYPTTESTNETLET_H_
#define _CRYPTTESTNETLET_H_

#include "simpleComposedNetlet.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

const std::string& CRYPT_TEST_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/cryptTestNetlet";

/**
 * @brief	A Netlet taking its actual building block configuration from
 * 			a generated source file.
 *
 * @TODO	Per architecture, a single template for a composable Netlet would
 * 			be sufficient.
 */
class CCryptTestNetlet : public CSimpleComposedNetlet
{
public:
	CCryptTestNetlet(CNena *nena, IMessageScheduler *sched);
	virtual ~CCryptTestNetlet();

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
class CCryptTestNetletMetaData : public INetletMetaData
{
public:
	CCryptTestNetletMetaData();
	virtual ~CCryptTestNetletMetaData();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;
	virtual int canHandle(const std::string& uri, std::string& req) const;

	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched); // factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLEMULTIMEDIANETLET_H_ */
