/** @file
 * netletSelector.h
 *
 * @brief Netlet repository interface.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Apr 30, 2009
 *      Author: denis
 */

#ifndef NETLETREPOSITORY_H_
#define NETLETREPOSITORY_H_

#include <netlet.h>
#include <netletMultiplexer.h>
#include <composableNetlet.h>
#include <exceptions.h>

#include <list>

#include <boost/shared_ptr.hpp>

class CNena;

#define EVENT_REPOSITORY_NETLETADDED 		"event://repository/netletAdded"
#define EVENT_REPOSITORY_MULTIPLEXERADDED 	"event://repository/multiplexerAdded"

/**
 * @brief	Interface for Netlet repository
 */
class INetletRepository : public IMessageProcessor
{
public:
	/**
	 * @brief	Thrown if the requested Netlet is unknown.
	 */
	NENA_EXCEPTION(ENoSuchNetlet);

	/**
	 * @brief	Thrown if the requested Netlet or Multiplexer is already loaded.
	 */
	NENA_EXCEPTION(EAlreadyLoaded);

	/**
	 * @brief	Thrown if the requested building block is unknown.
	 */
	NENA_EXCEPTION(ENoSuchBuildingBlock);

	/**
	 * @brief	Notification message when a new Netlet is added
	 */
	class Event_NetletAdded : public IEvent
	{
	public:
		std::string archId;
		std::string netletId;

		Event_NetletAdded(
				const std::string& archId,
				const std::string& netletId,
				IMessageProcessor *from = NULL, IMessageProcessor *to = NULL)
			: IEvent(from, to), archId(archId), netletId(netletId)
		{}

		virtual const std::string getId() const { return EVENT_REPOSITORY_NETLETADDED; }

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(new Event_NetletAdded(archId, netletId, from, to));
		}
	};

	/**
	 * @brief	Notification message when a new Netlet is added
	 */
	class Event_MultiplexerAdded : public IEvent
	{
	public:
		std::string archId;

		Event_MultiplexerAdded(
				const std::string& archId,
				IMessageProcessor *from = NULL, IMessageProcessor *to = NULL)
			: IEvent(from, to), archId(archId)
		{}

		virtual const std::string getId() const { return EVENT_REPOSITORY_MULTIPLEXERADDED; }

		virtual boost::shared_ptr<IEvent> clone() const
		{
			return boost::shared_ptr<IEvent>(new Event_MultiplexerAdded(archId, from, to));
		}
	};

protected:
	CNena *nena;

public:
	std::list<INetletMultiplexer *> multiplexers;	///< Instantiated, arch specific multiplexers (TODO: make protected)
	std::list<INetlet *> netlets;					///< Instantiated, arch specific Netlets (TODO: make protected)

	INetletRepository(CNena *nodeArch, IMessageScheduler *scheduler)
		: IMessageProcessor(scheduler), nena(nodeArch)
	{
	};

	virtual ~INetletRepository()
	{

	};

	/**
	 * @brief	Initialize the repository, e.g. load Netlets etc.
	 */
	virtual void initialize() = 0;

	/**
	 * @brief	Update the repository, e.g. load new Netlets etc.
	 */
	virtual void update() = 0;

	/**
	 * @brief	Load a specific Netlet into the active repository and notify the
	 * 			responsible multiplexer.
	 *
	 * @param name	Name of the Netlet to load (netlet://...)
	 *
	 * @return Pointer to the Netlet.
	 *
	 * @throw ENoSuchNetlet		Thrown if the requested Netlet is not found.
	 * @throw EAlreadyLoaded	Thrown of the requested Netlet is already loaded.
	 */
	virtual INetlet* loadNetlet(const std::string& name) throw (ENoSuchNetlet, EAlreadyLoaded) = 0;

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
	virtual bool unloadNetlet(const std::string& name) throw (ENoSuchNetlet) = 0;

	/**
	 * @brief return a new building block instance
	 *
	 * @param nena		pointer to nena interface
	 * @param sched		scheduler to use for bb
	 * @param netlet	netlet using the building block
	 * @param bbclass	class part of id
	 * @param id		unique id postfix for class string
	 *
	 * @return 		a shared pointer to the new building block
	 */
	virtual boost::shared_ptr<IBuildingBlock> buildingBlockFactory (CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string bbclass, const std::string id) = 0;

	virtual INetlet * getNetletByName (std::string name) = 0;

	/**
	 * @brief return an xml string containing stats
	 */
	virtual std::string getStats () = 0;
};

#endif /* NETLETREPOSITORY_H_ */
