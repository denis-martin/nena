/** @file
 * localRepository.h
 *
 * @brief Local Netlet repository.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: May 8, 2009
 *      Author: denis
 */

#ifndef LOCALREPOSITORY_H_
#define LOCALREPOSITORY_H_

#include "netletRepository.h"
#include "messages.h"
#include <composableNetlet.h>
#include <exceptions.h>

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <boost/extension/shared_library.hpp>
#include <boost/function.hpp>

/**
 * Multiplexer factory for self-registering of multiplexers.
 *
 * Note: Within omnet++, this factory class is only instantiated once for all
 * node architecture instances.
 *
 * Map index is the ID / name of the architecture
 */
extern MultiplexerFactories multiplexerFactories;

/**
 * Netlet factory for self-registering of Netlets
 *
 * Note: Within omnet++, this factory class is only instantiated once for all
 * node architecture instances.
 *
 * Index of first map is the architecture ID, index of second map is the Netlet ID
 */
extern NetletFactories netletFactories;

/**
 * List of names of simple architectures to be instantiated. For demo and test
 * purposes.
 */
extern std::list<std::string> simpleArchitectures;

class CNetletSelector;
class CNetAdaptBroker;

/**
 * @brief Local Netlet repository.
 *
 * Loads Netlets available as shared object files.
 */
class CLocalRepository : public INetletRepository
{
private:
	class SharedLib
	{
	public:
		SharedLib(std::string fileName, boost::extensions::shared_library *handle)
			 : fileName(fileName), handle(handle)
		{};
		virtual ~SharedLib() {};

		std::string fileName;
		boost::extensions::shared_library *handle;
	};

	std::list<SharedLib> loadedSharedLibraries;	///< Map of opened shared libraries

	class LoaderInfo {
	public:
		boost::function<boost::shared_ptr<IBuildingBlock> (CNena *, IMessageScheduler *, IComposableNetlet *, const std::string)> instantiateBuildingBlock;
		std::string libName;
	};

	std::map<std::string, LoaderInfo> loaderMap;

	/// we have to save these somewhere...
	std::list<boost::shared_ptr<INetletMetaData> > modelFactories;

	CNetletSelector* netletSelector;
	CNetAdaptBroker* netAdaptBroker;

	std::string defaultArch; // TODO quick hack, needs to be set more dynamically

protected:
//	std::string getSoFileNameX(const std::string soName);	///< Return shared object file name

	virtual bool dlibIsLoaded(const std::string& filename) const; ///< Check whether a specific shared library is already loaded
	virtual bool multiplexerIsLoaded(const std::string& name) const; ///< Check whether a specific multiplexer is already loaded
	virtual bool netletIsLoaded(const std::string& name) const; ///< Check whether a specific Netlet is already loaded	///< Load Netlet models
        
	virtual void loadBuildingBlocks();						///< Load available building blocks
	virtual void loadMultiplexers();						///< Load available multiplexers
	virtual void loadNetletLibraries();						///< Load available Netlet libraries
	virtual void loadModels();							///< Load models
	virtual void loadNetlets();							///< instantiate Netlets listed by config

public:

	NENA_EXCEPTION(EConfig);

	CLocalRepository(CNena *nodeArch, IMessageScheduler *sched);

	virtual ~CLocalRepository();

	/**
	 * @brief Initialize the repository, e.g. load Netlets etc.
	 */
	virtual void initialize();

	/**
	 * @brief Update the repository, e.g. load new Netlets etc.
	 */
	virtual void update();

	/**
	 * @brief	Load (instantiate) a specific Netlet into the active repository
	 * 			and notify the responsible multiplexer.
	 *
	 * @param name	Name of the Netlet to load (netlet://...)
	 *
	 * @return Pointer to the Netlet.
	 *
	 * @throw ENoSuchNetlet		Thrown if the requested Netlet is not found.
	 * @throw EAlreadyLoaded	Thrown of the requested Netlet is already loaded.
	 */
	virtual INetlet* loadNetlet(const std::string& name) throw (ENoSuchNetlet, EAlreadyLoaded);

	/**
	 * @brief	Remove a specific Netlet from the active repository
	 *
	 * @param name	Name of the Netlet to remove (netlet://...)
	 *
	 * @return 	True if Netlet was successfully removed, false if it could not be
	 * 			removed (i.e., it refuses to get removed).
	 *
	 * @throw ENoSuchNetlet	Thrown if the requested Netlet is not loaded.
	 */
	virtual bool unloadNetlet(const std::string& name) throw (ENoSuchNetlet);

	/**
	 * @brief instantiate a building block and return a pointer to it
	 *
	 * @param bbclass		the type of the building block, part of its URI
	 * @param bbid			postfix of its URI
	 */
	//virtual boost::shared_ptr<IBuildingBlock> createBuildingBlock (std::string bbclass, std::string bbid)

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {};

	/**
	 * @brief Process a timer message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {};

	/**
	 * @brief Process an outgoing message directed towards the network.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {};

	/**
	 * @brief Process an incoming message directed towards the application.
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage) {};

	virtual boost::shared_ptr<IBuildingBlock> buildingBlockFactory (CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string bbclass, const std::string id);

	virtual INetlet * getNetletByName (std::string name);

	/**
	 * @brief return an xml string containing stats
	 */
	virtual std::string getStats ();
        
	virtual const std::string & getId () const;
};

#endif /* LOCALREPOSITORY_H_ */
