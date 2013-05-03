/**@file
 * simpleReliableTransportNetlet.cpp
 *
 * @brief	Simple example of a composed transport Netlet
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 14, 2009
 *      Author: denis
 */

#include "simpleReliableTransportNetlet.h"

#include "nena.h"
#include "netAdaptBroker.h"
#include "messageBuffer.h"
#include "localRepository.h"

#include "edu.kit.tm/itm/sig/bb_restCommands.h"
#include "edu.kit.tm/itm/transport/arq/bb_arqGoBackN.h"
#include "edu.kit.tm/itm/transport/segment/bb_simpleSegment.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>

class TransportNetletConfig : public IComposableNetlet::Config
{
public:
	TransportNetletConfig()
	{
		outgoingChain.push_back("bb://edu.kit.tm/itm/transport/segment/simple");
		outgoingChain.push_back("bb://edu.kit.tm/itm/sig/restCommands");
		outgoingChain.push_back("bb://edu.kit.tm/itm/transport/arq/goBackN");

		// same for incoming
		std::list<std::string>::iterator it;
		for (it = outgoingChain.begin(); it != outgoingChain.end(); it++) {
			incomingChain.push_front(*it);
		}
	};

	virtual ~TransportNetletConfig()
	{
		outgoingChain.clear();
		incomingChain.clear();
	};
};

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

using namespace std;
using namespace edu_kit_tm::itm::transport;
using namespace edu_kit_tm::itm::sig;
using namespace boost::property_tree;
using boost::shared_ptr;

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleReliableTransportNetlet::CSimpleReliableTransportNetlet(CSimpleReliableTransportNetletMetaData* metaData,
		CNena *nena, IMessageScheduler *sched) :
	CSimpleComposedNetlet(nena, sched), metaData(metaData)
{
	className += "::CSimpleReliableTransportNetlet";
	setId(metaData->getId());

	DBG_DEBUG(FMT("%1% instantiated for %2%") % getId() % metaData->getArchName());

	config = new TransportNetletConfig();

	shared_ptr<IBuildingBlock> bb(new Bb_SimpleSegment(nena, sched, this, BB_SIMPLESEGMENT_ID + "/rt_netlet"));
	buildingBlocks[bb->getId()] = bb;

	bb.reset(new Bb_RestCommands(nena, sched, this, BB_RESTCOMMANDS_ID + "/rt_netlet"));
	buildingBlocks[bb->getId()] = bb;
	event_bb = bb.get();

	bb.reset(new Bb_ArqGoBackN(nena, sched, this, BB_ARQGOBACKN_ID + "/rt_netlet"));
	buildingBlocks[bb->getId()] = bb;

	rewire();
}

/**
 * Destructor
 */
CSimpleReliableTransportNetlet::~CSimpleReliableTransportNetlet()
{
	delete config;
	config = NULL;
	buildingBlocks.clear();
}

/**
 * @brief	Returns the Netlet's meta data
 */
INetletMetaData* CSimpleReliableTransportNetlet::getMetaData() const
{
	return (INetletMetaData*) metaData;
}

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleReliableTransportNetletMetaData::CSimpleReliableTransportNetletMetaData(const string& archName, const string& netletName) :
		archName(archName)
{
	netletId = archName;
	netletId.replace(0, netletId.find(':'), "netlet");

	if (!netletName.empty())
		netletId += "/" + netletName;
	else
		netletId += "/SimpleReliableTransportNetlet";

	MultiplexerFactories::iterator nit = multiplexerFactories.find(getArchName());
	if (nit == multiplexerFactories.end())
		DBG_ERROR(FMT("%1% cannot be instantiated: %2% not found") % getId() % getArchName());
	else
		netletFactories[getArchName()][getId()] = (INetletMetaData*) this;
}

/**
 * Destructor
 */
CSimpleReliableTransportNetletMetaData::~CSimpleReliableTransportNetletMetaData()
{
	NetletFactories::iterator ait = netletFactories.find(getArchName());
	if (ait != netletFactories.end()) {
		map<string, INetletMetaData*>::iterator nit= ait->second.find(getId());
		if (nit != ait->second.end()) {
			ait->second.erase(nit);
		}
	}

}

/**
 * Return Netlet name
 */
std::string CSimpleReliableTransportNetletMetaData::getArchName() const
{
	return archName;
}

/**
 * Return Netlet name
 */
const std::string& CSimpleReliableTransportNetletMetaData::getId() const
{
	return netletId;
}

/**
 * @brief	Returns true if the Netlet does not offer any transport service
 * 			for applications, false otherwise.
 */
bool CSimpleReliableTransportNetletMetaData::isControlNetlet() const
{
	return false;
}

/**
 * @brief	Returns confidence value for the given name (uri) and the given
 * 			requirements (req). A value of 0 means, that the Netlet cannot
 * 			handle the request at all.
 */
int CSimpleReliableTransportNetletMetaData::canHandle(const std::string& uri, std::string& req) const
{
	string ns = uri.substr(0, uri.find(':'));
	ptree reqpt;

	if (!req.empty()) {
		try {
			stringstream reqs(req);
			read_json(reqs, reqpt);

		} catch (json_parser_error& jpe) {
			DBG_DEBUG(FMT("%1%: ERROR parsing requirement string: %2%") % getId() % jpe.what());
			return 0;

		}

	}

	if (properties.find("namespaces")->second.find(string("|") + ns + "|") != string::npos) {
		// we know that namespace

		// check all requirements against our properties
		int matchedAll = 1;
		ptree::iterator it;
		for (it = reqpt.begin(); it != reqpt.end(); it++) {
			ptree::value_type v = *it;
			map<string, string>::const_iterator pit = properties.find(v.first);
			if (pit != properties.end()) {
				if (pit->second.compare(v.second.data()) == 0) {
					DBG_INFO(FMT("%1%: requirement %2% = %3% matched") % getId() % v.first % v.second.data());

				} else {
					DBG_INFO(FMT("%1%: requirement %2% = %3% FAILED (property value mismatch)") % getId() % v.first % v.second.data());
					matchedAll = 0;

				}

			} else {
				DBG_INFO(FMT("%1%: requirement %2% = %3% FAILED (property unknown)") % getId() % v.first % v.second.data());
				matchedAll = 0;

			}

		}

		if (matchedAll)
			DBG_DEBUG(FMT("%1%: canHandle: meeting all requirements") % getId());

		return matchedAll;

	}

	return 0;
}

/**
 * Create an instance of the Netlet
 */
INetlet* CSimpleReliableTransportNetletMetaData::createNetlet(CNena *nodeA, IMessageScheduler *sched)
{
	if (properties.empty()) {
		if (nodeA->getConfig()->hasParameter(getId(), "properties", XMLFile::STRING, XMLFile::TABLE)) {
			vector<vector<string> > props;
			nodeA->getConfig()->getParameterTable(getId(), "properties", props);

			vector<vector<string> >::const_iterator oit;
			for (oit = props.begin(); oit != props.end(); oit++)
				properties[(*oit)[0]] = (*oit)[1];

		}

		if (properties["namespaces"].empty())
			properties["namespaces"] = "|node|app|";

		if (properties["reliable"].empty())
			properties["reliable"] = "1";

		DBG_DEBUG(FMT("%1%: properties") % getId());
		map<string, string>::const_iterator pit;
		for (pit = properties.begin(); pit != properties.end(); pit++) {
			DBG_DEBUG(FMT("  %1% = %2%") % pit->first % pit->second);
		}
	}

	return new CSimpleReliableTransportNetlet(this, nodeA, sched);
}

/**
 * @brief	Return static Netlet property
 */
std::string CSimpleReliableTransportNetletMetaData::getProperty(const std::string& prop) const
{
	map<std::string, std::string>::const_iterator it = properties.find(prop);
	if (it != properties.end())
		return it->second;

	return string();
}


/* ========================================================================= */

/**
 * @brief	For demo-purposes and tests: instantiate multiple architectures
 */
class CSimpleReliableTransportNetletMultiplier
{
public:
	list<CSimpleReliableTransportNetletMetaData*> factories;

	CSimpleReliableTransportNetletMultiplier()
	{
		list<string>::const_iterator it;
		for (it = simpleArchitectures.begin(); it != simpleArchitectures.end(); it++) {
			factories.push_back(new CSimpleReliableTransportNetletMetaData(*it));
			if ((*it) == "architecture://video")
				factories.push_back(new CSimpleReliableTransportNetletMetaData(*it, "SimpleControlTransportNetlet"));

		}
	}

	virtual ~CSimpleReliableTransportNetletMultiplier()
	{
		list<CSimpleReliableTransportNetletMetaData*>::iterator it;
		for (it = factories.begin(); it != factories.end(); it++) {
			delete *it;
		}
		factories.clear();
	}
};

/**
 * Initializer for shared library. Registers factories.
 */
static CSimpleReliableTransportNetletMultiplier simpleReliableTransportNetletMultiplier;

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
