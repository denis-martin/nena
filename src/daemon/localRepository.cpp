/** @file
 * localRepository.cpp
 *
 * @brief Local Netlet repository.
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: May 8, 2009
 *      Author: denis
 */

// v3 was introduced in 1.44, thus we need at least boost v1.44
#define BOOST_FILESYSTEM_VERSION 3

#include "localRepository.h"

#include "nena.h"
#include "netletSelector.h"
#include "netAdaptBroker.h"
#include "modelBased/netletTemplate.h"

//#include "modelBased/modelBasedNetlet.h"

#include "xmlNode/xmlNode.h"        
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>

// draw-back of boost::extensions - it does not ship anything like dlerror()
// TODO: this won't work on Windows
#include <dlfcn.h>
#include <stdio.h>

#include <sstream>
#include <vector>
#include <set>

/**
 * Multiplexer factory for self-registering of multiplexers.
 *
 * Note: Within omnet++, this factory class is only instantiated once for all
 * node architecture instances.
 *
 * Map index is the ID / name of the architecture
 */
MultiplexerFactories multiplexerFactories;

/**
 * Netlet factory for self-registering of Netlets
 *
 * Note: Within omnet++, this factory class is only instantiated once for all
 * node architecture instances.
 *
 * Index of first map is the architecture ID, index of second map is the Netlet ID
 */
NetletFactories netletFactories;

/**
 * List of names of simple architectures to be instantiated. For demo and test
 * purposes.
 */
std::list<std::string> simpleArchitectures;

using namespace std;
using namespace boost;

const string localRepositoryId = "internalservice://nena/repository";

CLocalRepository::CLocalRepository(CNena *nodeArch, IMessageScheduler *sched)
	: INetletRepository(nodeArch, sched), netletSelector(NULL)
{
	setId(localRepositoryId);

	registerEvent(EVENT_REPOSITORY_NETLETADDED);
	registerEvent(EVENT_REPOSITORY_MULTIPLEXERADDED);

	nena->registerInternalService(getId(), this);

	if (nena->getConfig()->hasParameter(getId(), "simpleArchitectures", XMLFile::STRING, XMLFile::LIST)) {
		vector<string> simpleArchs;
		nena->getConfig()->getParameterList(getId(), "simpleArchitectures", simpleArchs);
		simpleArchitectures.assign(simpleArchs.begin(), simpleArchs.end());
		assert(simpleArchs.size() == simpleArchitectures.size());

	} else {
		simpleArchitectures.push_back("architecture://edu.kit.tm/itm/simpleArch");

	}
}

CLocalRepository::~CLocalRepository()
{
	DBG_DEBUG("LR: Destroying Netlets");
	list<INetlet *>::iterator n_it;
	for (n_it = netlets.begin(); n_it != netlets.end(); n_it++)
		delete *n_it;
	netlets.clear();

	DBG_DEBUG("LR: Destroying Multiplexers");
	list<INetletMultiplexer *>::iterator nm_it;
	for (nm_it = multiplexers.begin(); nm_it != multiplexers.end(); nm_it++)
		delete *nm_it;
	multiplexers.clear();

	// make sure, that Netlets are closed before their multiplexers are closed
	DBG_DEBUG("LR: Closing shared libraries");
	std::list<SharedLib>::reverse_iterator dl_it;
	for (dl_it = loadedSharedLibraries.rbegin(); dl_it != loadedSharedLibraries.rend(); dl_it++)
		delete dl_it->handle;
	loadedSharedLibraries.clear();
}

/**
 * @brief Initialize the repository, e.g. load Netlets etc.
 */
void CLocalRepository::initialize()
{
	netletSelector = (CNetletSelector*) nena->lookupInternalService("internalservice://nena/netletSelector");
	netAdaptBroker = (CNetAdaptBroker*) nena->lookupInternalService("internalservice://nena/netAdaptBroker");
	update();
}

/**
 * @brief Update the repository, e.g. load new Netlets etc.
 */
void CLocalRepository::update()
{
	DBG_DEBUG("LR: Loading building blocks");
	loadBuildingBlocks();

	DBG_DEBUG("LR: Loading multiplexers");
	loadMultiplexers();

	DBG_DEBUG("LR: Loading Netlets");
	loadNetletLibraries();

	DBG_DEBUG("LR: Loading Models");
	loadModels();

	DBG_DEBUG("LR: Instantiating Netlets");
	loadNetlets();

//	INetlet* test = new CModelBasedNetlet(nena, scheduler);
}

///**
// * @brief Get OS dependent file name of shared object (library)
// *
// * TODO: Only UNIX type naming is supported. Windows (*.dll) and others
// *       should be added.
// *
// * @param soName	Name of shared object (e.g. name of netlet)
// */
//std::string CLocalRepository::getSoFileNameX(const std::string soName)
//{
//	return (FMT("../../netlets/lib%1%.so") % soName).str();
//}

/**
 * @brief 	Check whether a specific shared library is already loaded
 */
bool CLocalRepository::dlibIsLoaded(const std::string& filename) const
{
	// sequential lookup sufficient for now
	std::list<SharedLib>::const_iterator it;
	for (it = loadedSharedLibraries.begin(); it != loadedSharedLibraries.end(); it++) {
		if (it->fileName == filename)
			return true;
	}

	return false;
}

/**
 * @brief 	Check whether a specific multiplexer is already loaded
 */
bool CLocalRepository::multiplexerIsLoaded(const std::string& name) const
{
	// sequential lookup sufficient for now
	std::list<INetletMultiplexer*>::const_iterator it;
	for (it = multiplexers.begin(); it != multiplexers.end(); it++) {
		if ((*it)->getMetaData()->getArchName() == name)
			return true;
	}

	return false;
}

/**
 * @brief	Check whether a specific Netlet is already loaded
 */
bool CLocalRepository::netletIsLoaded(const std::string& name) const
{
	// sequential lookup sufficient for now
	std::list<INetlet*>::const_iterator it;
	for (it = netlets.begin(); it != netlets.end(); it++) {
		if ((*it)->getMetaData()->getId() == name)
			return true;
	}

	return false;
}

void CLocalRepository::loadBuildingBlocks()
{
	string pathstr;

	if (!nena->getConfig()->hasParameter(localRepositoryId, "buildingBlocksDirectory"))
		pathstr =  "../../buildingBlocks/";
	else
		nena->getConfig()->getParameter(localRepositoryId, "buildingBlocksDirectory", pathstr);

	filesystem::path sopath(pathstr);

	filesystem::recursive_directory_iterator dirit_end;
	filesystem::recursive_directory_iterator dirit(sopath);
	for (/* nothing */; dirit != dirit_end; ++dirit)
	{
		if (is_directory(dirit->status()) && dirit->path().filename().c_str()[0] == '.')
		{
			DBG_DEBUG(FMT("LR: skipping %1%") % dirit->path().filename());
			dirit.no_push();
		}

		if (/*dirit->filename().find("multiplexer") != string::npos && */
				dirit->path().extension() == ".so" &&
				!dlibIsLoaded(dirit->path().filename().native()))
		{
			string dlibName = dirit->path().string();
			DBG_DEBUG(FMT("LR: Loading %1%") % dlibName);

			extensions::shared_library *dlib = new extensions::shared_library(dlibName, true);
			if (!dlib->open()) {
				// TODO: dlerror() won't work on Windows
				DBG_FAIL(FMT("Failed to open %1%: %2%") % dlibName % dlerror());

			} else {
				// for closing it later
				loadedSharedLibraries.push_back(SharedLib(dirit->path().filename().native(), dlib));

			}

			LoaderInfo li;
			function<set<string> (void)> getBuildingBlockClasses(dlib->get<set<string> >("getBuildingBlockClasses"));

			set<string> bbnames = getBuildingBlockClasses ();

			li.libName = dlibName;

			for (set<string>::const_iterator it=bbnames.begin(); it != bbnames.end(); it++) {
				map<string, LoaderInfo>::const_iterator lit = loaderMap.find(*it);
				if (lit != loaderMap.end()) {
					DBG_WARNING(FMT("LR: Building Block '%1%' already supplied by library '%2%', unable to load from '%3%'") % *it % lit->second.libName % dlibName);
				} else {
					DBG_INFO(FMT("LR: lib provides bb '%1%'") % *it);
					li.instantiateBuildingBlock = boost::bind(dlib->get<shared_ptr<IBuildingBlock>, CNena *, IMessageScheduler *, IComposableNetlet *, const string, const string>("instantiateBuildingBlock"), _1, _2, _3, *it, _4);
					loaderMap[*it] = li;
				}
			}

		}
	}
}

/**
 * @brief	Scan for and load multiplexers
 */
void CLocalRepository::loadMultiplexers()
{
	string pathstr;

	if (!nena->getConfig()->hasParameter(localRepositoryId, "archsDirectory"))
		pathstr =  "../../archs/";
	else
		nena->getConfig()->getParameter(localRepositoryId, "archsDirectory", pathstr);

	filesystem::path sopath(pathstr);

	filesystem::directory_iterator dirit_end;
	filesystem::directory_iterator dirit(sopath);
	for (/* nothing */; dirit != dirit_end; ++dirit)
	{
		if (dirit->path().filename().native().find("multiplexer") != string::npos &&
				dirit->path().filename().extension() == ".so" &&
				!dlibIsLoaded(dirit->path().filename().native()))
		{
			string dlibName = dirit->path().native();
			DBG_DEBUG(FMT("LR: Loading %1%") % dlibName);

			extensions::shared_library *dlib = new extensions::shared_library(dlibName, true);
			if (!dlib->open()) {
				// TODO: dlerror() won't work on Windows
				DBG_FAIL(FMT("Failed to open %1%: %2%") % dlibName % dlerror());

			} else {
				// for closing it later
				loadedSharedLibraries.push_back(SharedLib(dirit->path().filename().native(), dlib));

			}

		}
	}

	// instantiate multiplexers
	MultiplexerFactories::iterator mit;
	for (mit = multiplexerFactories.begin(); mit != multiplexerFactories.end(); mit++) {

		if (!multiplexerIsLoaded((mit->second->getArchName()))) {
			DBG_DEBUG(FMT("LR: Instantiating architecture %1%") % mit->second->getArchName());
			multiplexers.push_back(mit->second->createMultiplexer(nena, nena->getDefaultScheduler()));

			// attach NAs to multiplexer
			list<INetAdapt *>& nas = netAdaptBroker->getNetAdapts(mit->second->getArchName());
			if (nas.size() == 0) {
				multiplexers.back()->setNext(NULL);
				DBG_WARNING(FMT("No network adaptors for architecture %1% found.") %
						multiplexers.back()->getMetaData()->getArchName());

			} else {
				list<INetAdapt *>::iterator nas_it;
				for (nas_it = nas.begin(); nas_it != nas.end(); nas_it++)
					(*nas_it)->setPrev(multiplexers.back());

				// TODO: determine some sort of default network access
				multiplexers.back()->setNext(nas.front());

			}

			notifyListeners(Event_MultiplexerAdded(mit->second->getArchName()));
		}
	}

}

void CLocalRepository::loadNetletLibraries()
{
	string pathstr;

	if (!nena->getConfig()->hasParameter(localRepositoryId, "netletsDirectory"))
		pathstr =  "../../netlets/";
	else
		nena->getConfig()->getParameter(localRepositoryId, "netletsDirectory", pathstr);

	filesystem::path sopath(pathstr);

	filesystem::directory_iterator dirit_end;
	filesystem::directory_iterator dirit(sopath);
	for (/* nothing */; dirit != dirit_end; ++dirit)
	{
		// TODO: add windows support
		//DBG_DEBUG(FMT("LR: File %1%, extension %2%") % dirit->filename() % dirit->path().extension());
		if (dirit->path().filename().native().find("netlet") != string::npos &&
				dirit->path().filename().extension() == ".so" &&
				!dlibIsLoaded(dirit->path().filename().native()))
		{
			string dlibName = dirit->path().native();
			DBG_DEBUG(FMT("LR: Loading %1%") % dlibName);

			extensions::shared_library *dlib = new extensions::shared_library(dlibName, true);
			if (!dlib->open()) {
				// TODO: dlerror() won't work on Windows
				DBG_WARNING(FMT("Failed to open %1%: %2%") % dlibName % dlerror());
				delete dlib;
				dlib = NULL;

			} else {
				// for closing it later
				loadedSharedLibraries.push_back(SharedLib(dirit->path().filename().native(), dlib));

			}

		}
	}
}

void CLocalRepository::loadNetlets()
{
	// instantiate all Netlets defined in the config file or all,
	// if the respective config section is empty
	vector<string> netletsToLoad;
	nena->getConfig()->getParameterList(localRepositoryId, "netletsToLoad", netletsToLoad);

	NetletFactories::const_iterator nit;
	for (nit = netletFactories.begin(); nit != netletFactories.end(); nit++)
	{
		const string& archName = nit->first;

		map<string, INetletMetaData*>::const_iterator nit2;
		for (nit2 = nit->second.begin(); nit2 != nit->second.end(); nit2++)
		{
			const string& netletName = nit2->first;

			bool instantiateNetlet = false;
			if (!netletsToLoad.empty())
			{
				for (vector<string>::const_iterator it = netletsToLoad.begin(); it != netletsToLoad.end(); ++it)
				{
					if (*it == netletName)
					{
						instantiateNetlet = true;
						break;
					}
				}

			}
			else
			{
				// if no netlets are given, instantiate all
				// TODO defer instantiation upon request
				instantiateNetlet = true;
			}

			// check whether Netlet is already instantiated
			list<INetlet*>::const_iterator it;
			for (it = netlets.begin(); it != netlets.end() && instantiateNetlet; it++)
			{
					if ((*it)->getMetaData()->getId() == netletName)
					{
							instantiateNetlet = false;
							DBG_INFO(FMT("LR: %1% already instantiated") % netletName);
							break;
					}
			}

			if (instantiateNetlet)
			{
				/// take the specified scheduler, default otherwise
				IMessageScheduler * sched;

				if (nena->getConfig()->hasParameter (netletName, "scheduler"))
				{
					string schedulerName;
					nena->getConfig()->getParameter (netletName, "scheduler", schedulerName);

					sched = nena->getSchedulerByName(schedulerName);
					if (sched == NULL)
					{
						sched = nena->getDefaultScheduler();
						DBG_WARNING(FMT("Scheduler %1% cannot be found. Default scheduler will be used for netlet %2%.") % schedulerName % netletName);
					}
				}
				else
					sched = nena->getDefaultScheduler();

				NetletFactories::iterator arch_it = netletFactories.find(archName);

				if (arch_it == netletFactories.end())
					DBG_FAIL(FMT("Arch factories not found!"));

				map<string, INetletMetaData*>::iterator netlet_it = arch_it->second.find(netletName);

				if (netlet_it == arch_it->second.end())
					DBG_FAIL(FMT("Netlet factory not found!"));

				INetlet * nl = netlet_it->second->createNetlet(nena, sched);

				netlets.push_back(nl);
				netlets.back()->setPrev(netletSelector);
				// attachment Netlet -> multiplexer is done by INetletMultiplexer::refreshNetlets()

				notifyListeners(Event_NetletAdded(archName, netletName));
			}
			else
			{
				DBG_INFO(FMT("LR: Omitting instantiation of %1%") % netletName);
			}

		}

	}

	// notify multiplexers on changes (TODO: optimize this?!)
	list<INetletMultiplexer *>::const_iterator mit;
	for (mit = multiplexers.begin(); mit != multiplexers.end(); mit++)
		(*mit)->refreshNetlets();
}

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
INetlet* CLocalRepository::loadNetlet(const std::string& name) throw (ENoSuchNetlet, EAlreadyLoaded)
{
	INetlet* netlet = NULL;

	// sanity check
	list<INetlet*>::const_iterator n_it;
	for (n_it = netlets.begin(); n_it != netlets.end(); n_it++) {
		if ((*n_it)->getMetaData()->getId() == name)
			throw EAlreadyLoaded();
	}

	NetletFactories::iterator f_it;
	for (f_it = netletFactories.begin(); f_it != netletFactories.end(); f_it++) {
		if (f_it->second.find(name) != f_it->second.end()) {
			netlet = f_it->second[name]->createNetlet(nena, nena->getDefaultScheduler());
			netlets.push_back(netlet);

			// notify concerned multiplexer of the change
			// TODO: event based?
			list<INetletMultiplexer*>::iterator m_it;
			for (m_it = multiplexers.begin(); m_it != multiplexers.end(); m_it++) {
				if ((*m_it)->getMetaData()->getArchName() == f_it->first) {
					(*m_it)->refreshNetlets();
					break;
				}
			}

			break;
		}
	}

	if (netlet == NULL)
		throw ENoSuchNetlet();

	return netlet;
}

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
bool CLocalRepository::unloadNetlet(const std::string& name) throw (ENoSuchNetlet)
{
	list<INetlet *>::iterator n_it;
	for (n_it = netlets.begin(); n_it != netlets.end(); n_it++) {
		if ((*n_it)->getMetaData()->getId() == name) {
			// TODO: well, the Netlet isn't really asked - need to gracefully remove it
			netlets.erase(n_it);
			delete *n_it;
			return true;
		}
	}

	throw ENoSuchNetlet();

	// return value false is reserved if Netlet refuses to get removed
	return false;
}

/**
 * @brief	Parse available Netlet models and prepare them for later
 * 			instantiation.
 */
void CLocalRepository::loadModels()
{
	using edu_kit_tm::itm::generic::CNetletMetaDataTemplate;

	vector<string> netletsToModel;
	if (!nena->getConfig()->hasParameter(localRepositoryId, "netletsToModel", XMLFile::STRING, XMLFile::LIST))
		return;

	nena->getConfig()->getParameterList(localRepositoryId, "netletsToModel", netletsToModel);

	for (vector<string>::const_iterator it=netletsToModel.begin(); it != netletsToModel.end(); it++) {
		DBG_INFO(FMT("LR: Loading model for netlet '%1%'") % *it);
		shared_ptr<INetletMetaData> ptr(new CNetletMetaDataTemplate(*it, nena));
		modelFactories.push_back(ptr);
	}
}

///**
// * @brief	Parse a given model (work in progress)
// */
//void CLocalRepository::parseModel(std::istream& stream)
//{
//	const string KEY_NETLETMODEL 	= "edu.kit.tm.intend.models.NetletModel";
//	const string KEY_DATASOURCE 	= "edu.kit.tm.intend.models.DataStreamSourceModel";
//	const string KEY_DATASINK	 	= "edu.kit.tm.intend.models.DataStreamSinkModel";
//	const string KEY_CONNMODEL	 	= "edu.kit.tm.intend.models.ConnectionModel";
//	const string KEY_OUGOING 		= "outgoingConnections";
//	const string KEY_TARGET 		= "target";
//	const string KEY_IDENTIFIER		= "identifier";
//
//	sxml::XmlNode rootNode;
//	rootNode.readFromStream(stream);
//
//	if (!(rootNode.type == sxml::ntElementNode && rootNode.name == KEY_NETLETMODEL)) {
//		DBG_DEBUG(FMT("LR: Error parsing Netlet model: Invalid root node \"%1%\"") % rootNode.name);
//		return;
//	}
//
//	sxml::XmlNode* nmodel = NULL;
//	sxml::NodeChildrenIterator ch_it;
//	for (ch_it = rootNode.children.begin(); ch_it != rootNode.children.end(); ch_it++) {
//		sxml::XmlNode* chnode = *ch_it;
//		if (chnode->type == sxml::ntElementNode && chnode->name == KEY_NETLETMODEL) {
//			nmodel = chnode;
//			break;
//		}
//	}
//	if (nmodel == NULL) {
//		DBG_DEBUG(FMT("LR: Error parsing Netlet model: Missing sub node \"%1%\"") % KEY_NETLETMODEL);
//		return;
//	}
//
//	sxml::XmlNode* nmodel_def = nmodel->findFirst("default");
//	if (nmodel_def == NULL) {
//		DBG_DEBUG(FMT("LR: Error parsing Netlet model: Missing sub node \"%1%\"") % "default");
//		return;
//	}
//
//	string netletId;
//	string archId;
//	for (ch_it = nmodel_def->children.begin(); ch_it != nmodel_def->children.end(); ch_it++) {
//		sxml::XmlNode* chnode = *ch_it;
//		if (chnode->type == sxml::ntElementNode && chnode->name == "id") {
//			if (chnode->children.size() == 0 || chnode->children[0]->type != sxml::ntTextNode) {
//				DBG_DEBUG(FMT("LR: Error parsing Netlet model: Invalid sub node \"%1%\"") % "id");
//				return;
//			}
//
//			netletId = chnode->children[0]->name;
//
//		} else if (chnode->type == sxml::ntElementNode && chnode->name == "archId") {
//			if (chnode->children.size() == 0 || chnode->children[0]->type != sxml::ntTextNode) {
//				DBG_DEBUG(FMT("LR: Error parsing Netlet model: Invalid sub node \"%1%\"") % "archId");
//				return;
//			}
//
//			archId = chnode->children[0]->name;
//
//		}
//	}
//
//	if (netletId.empty()) {
//		DBG_DEBUG(FMT("LR: Error parsing Netlet model: no Netlet ID found"));
//		return;
//	}
//
//	if (archId.empty()) {
//		DBG_DEBUG(FMT("LR: Error parsing Netlet model: no architecture ID found"));
//		return;
//	}
//
//	DBG_DEBUG(FMT("LR: Netlet %1%") % netletId);
//	DBG_DEBUG(FMT("LR:   Arch %1%") % archId);
//
//	sxml::XmlNode* dsmodel = nmodel_def->findFirst(KEY_DATASOURCE);
//	if (dsmodel == NULL) {
//		DBG_DEBUG(FMT("LR: Error parsing Netlet model: Missing sub node \"%1%\"") % KEY_DATASOURCE);
//		return;
//	}
//
//	sxml::NodeSearch* outns = dsmodel->findInit(KEY_OUGOING);
//	sxml::XmlNode* node;
//	while ((node = dsmodel->findNext(outns))) {
//		sxml::XmlNode* cmodel = node->findFirst(KEY_CONNMODEL);
//		if (cmodel == NULL) {
//			DBG_DEBUG(FMT("LR: Error parsing Netlet model: Missing sub node \"%1%\"") % KEY_CONNMODEL);
//			break;
//		}
//
//		sxml::XmlNode* tmodel = node->findFirst(KEY_TARGET);
//		if (tmodel == NULL) {
//			DBG_DEBUG(FMT("LR: Error parsing Netlet model: Missing sub node \"%1%\"") % KEY_TARGET);
//			break;
//		}
//
//		if (tmodel->attributes["class"] == KEY_DATASINK) {
//			// we're done
//			break;
//		}
//
//		sxml::XmlNode* id = node->findFirst(KEY_IDENTIFIER);
//		if (id == NULL) {
//			DBG_DEBUG(FMT("LR: Error parsing Netlet model: Missing sub node \"%1%\"") % KEY_IDENTIFIER);
//			break;
//		}
//
//		if (id->children.size() == 0 || id->children[0]->type != sxml::ntTextNode) {
//			DBG_DEBUG(FMT("LR: Error parsing Netlet model: Invalid sub node \"%1%\"") % KEY_IDENTIFIER);
//			break;
//		}
//
//		DBG_DEBUG(FMT("LR:     BB %1%") % id->children[0]->name);
//	}
//
//	dsmodel->findFree(outns);
//}

shared_ptr<IBuildingBlock> CLocalRepository::buildingBlockFactory (CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string bbclass, const std::string id)
{
	if (loaderMap.find(bbclass) == loaderMap.end())
		throw ENoSuchBuildingBlock(bbclass);
	return loaderMap[bbclass].instantiateBuildingBlock(nena, sched, netlet, id);
}

INetlet * CLocalRepository::getNetletByName (string name)
{
	list<INetlet *>::iterator n_it;
	for (n_it = netlets.begin(); n_it != netlets.end(); n_it++)
	{
		if ((*n_it)->getId() == name)
			return (*n_it);
	}
	return NULL;
}



string CLocalRepository::getStats ()
{
//	xml_document stats;
	string ret;
//	ostringstream oss;
//
//	stats.append_child ("stats").append_child ("component").append_attribute("name").set_value("repository");
//
//	/**
//	 * archs
//	 */
//
//	xml_node archlist = stats.child("stats").find_child_by_attribute("component", "name", "repository").append_child("archList");
//
//	NetletFactories::const_iterator nit;
//	for (nit = netletFactories.begin(); nit != netletFactories.end(); nit++)
//	{
//		const string & archName = nit->first;
//		xml_node li = archlist.append_child("li");
//		li.append_child("url").append_child(node_pcdata).set_value(archName.c_str());
//	}
//
//	/**
//	 * then all avaiable netlets
//	 */
//
//	xml_node allNetletlist = stats.child("stats").find_child_by_attribute("component", "name", "repository").append_child("allNetletList");
//
//	for (nit = netletFactories.begin(); nit != netletFactories.end(); nit++)
//	{
//		const string& archName = nit->first;
//
//		map<string, INetletMetaData*>::const_iterator nit2;
//		for (nit2 = nit->second.begin(); nit2 != nit->second.end(); nit2++)
//		{
//			const string & netletName = nit2->first;
//			xml_node li = allNetletlist.append_child("li");
//			li.append_child("url").append_child(node_pcdata).set_value(netletName.c_str());
//			li.append_child("arch").append_child(node_pcdata).set_value(archName.c_str());
//		}
//	}
//
//	/**
//	 * and the loaded netlets
//	 */
//
//	xml_node loadedNetletlist = stats.child("stats").find_child_by_attribute("component", "name", "repository").append_child("loadedNetletList");
//
//	list<INetlet *>::iterator it;
//
//	for (it = netlets.begin(); it != netlets.end(); it++)
//	{
//		string netletName = (*it)->getId();
//		xml_node li = loadedNetletlist.append_child("li");
//		li.append_child("url").append_child(node_pcdata).set_value(netletName.c_str());
//	}
//
//	stats.print(oss, "", format_raw);
//	ret = oss.str();
	return ret;
}

const std::string & CLocalRepository::getId () const
{
	return localRepositoryId;
}
