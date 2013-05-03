/** @file
 * netletMultiplexer.h
 *
 * @brief Generic interface for architecture specific multiplexers and their meta data.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#ifndef NETLETMULTIPLEXER_H_
#define NETLETMULTIPLEXER_H_

#include "messages.h"
#include "debug.h"

#include <map>
#include <string>

class IMultiplexerMetaData;
class CNena;

/**
 * @brief Generic Netlet multiplexer interface.
 */
class INetletMultiplexer : public IMessageProcessor
{
protected:
	/**
	 * @brief Pointer to meta data
	 */
	IMultiplexerMetaData *meta;
	CNena *nodeArch;

public:
	INetletMultiplexer(IMultiplexerMetaData *metaData, CNena *nodeA, IMessageScheduler *sched) :
		IMessageProcessor(sched), meta(metaData), nodeArch(nodeA)
	{
		className += "::INetletMultiplexer";
	};

	virtual ~INetletMultiplexer()
	{
	};

	/**
	 * @brief Return pointer to meta data
	 */
	IMultiplexerMetaData *getMetaData() { return meta; };

	/**
	 * @brief	Called if new Netlets of this architecture were added to the system.
	 */
	virtual void refreshNetlets() = 0;

	/**
	 * @brief return an xml string containing stats
	 */
	virtual std::string getStats () = 0;

};

/**
 * @brief Multiplexer meta data.
 *
 * The multiplexer meta data class has two purposes: On one hand, it describes
 * the architecure's properties, on the other hand, it provides a factory
 * functions that will be used by the node architecture daemon to instantiate
 * new "architecture".
 */
class IMultiplexerMetaData
{
public:
	IMultiplexerMetaData() {};
	virtual ~IMultiplexerMetaData() {};

	/**
	 * @brief Return the name of the architecture.
	 */
	virtual std::string getArchName() const = 0;

	/**
	 * @brief Factory function.
	 *
	 * Returns an instance of the architecture specific multiplexer. On a
	 * single node, there should be only ONE multiplexer per architecture.
	 */
	virtual INetletMultiplexer* createMultiplexer(CNena *nodeA, IMessageScheduler *sched) = 0; // factory function
};

/// Type for global collection of multiplexer factories.
typedef std::map<std::string, IMultiplexerMetaData*> MultiplexerFactories;

/// Global collection of multiplexer factories (allocated in Node Arch Daemon).
extern MultiplexerFactories multiplexerFactories;

#endif /* NETLETMULTIPLEXER_H_ */
