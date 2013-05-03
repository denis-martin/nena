/** @file
 * simpleNetlet.h
 *
 * @brief Simple example Netlet
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#ifndef SIMPLENETLET_H_
#define SIMPLENETLET_H_

#include "netlet.h"
#include "netletSelector.h"

#include "edu.kit.tm/itm/simpleArch/simpleMultiplexer.h"

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

const std::string& SIMPLE_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/SimpleNetlet";

/**
 * @brief Simple Netlet example
 */
class CSimpleNetlet : public INetlet
{
protected:
	CNetletSelector* netletSelector;

	std::map<unsigned int, std::string> services;

public:
	CSimpleNetlet(CNena *nena, IMessageScheduler *sched);
	virtual ~CSimpleNetlet();

	// from IMessageProcessor

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/**
	 * @brief Process an incoming message directed towards the application.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	// from INetlet

	/**
	 * @brief	Returns the Netlet's meta data
	 */
	virtual INetletMetaData* getMetaData() const;

	virtual const std::string & getId () const { return getMetaData()->getId(); }
};

/**
 * @brief Simple Netlet meta data
 *
 * The Netlet meta data class has two purposes: On one hand, it describes
 * the Netlet's properties, capabilities, etc., on the other hand, it provides
 * a factory functions that will be used by the node architecture daemon to
 * instantiate new Netlets of this type.
 */
class CSimpleNetletMetaData : public INetletMetaData
{
public:
	CSimpleNetletMetaData();
	virtual ~CSimpleNetletMetaData();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;
	virtual int canHandle(const std::string& uri, std::string& req) const;

	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched); // factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* SIMPLENETLET_H_ */
