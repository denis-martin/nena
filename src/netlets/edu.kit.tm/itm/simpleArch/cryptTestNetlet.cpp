/**@file
 * cryptTestNetlet.cpp
 *
 * @brief	Netlet which provides AES encryption for testing purposes
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
*  Created on: April 18, 2010
 *      Author: Benjamin Behringer
 */

#include "cryptTestNetlet.h"

#include "nena.h"
#include "netAdaptBroker.h"
#include "messageBuffer.h"

#include "configs/bbcfg_CryptTestNetlet.h"

#include <map>

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

using namespace std;

using edu_kit_tm::itm::crypt::Bb_Frag;
using edu_kit_tm::itm::crypt::Bb_Header;
using edu_kit_tm::itm::crypt::Bb_Pad;
using edu_kit_tm::itm::crypt::Bb_Enc;
using edu_kit_tm::itm::crypt::Bb_CRC;

/**
 * Initializer for shared library. Registers factory function.
 */
static CCryptTestNetletMetaData cryptTestNetletMetaData;

/* ========================================================================= */

/**
 * Constructor
 */
CCryptTestNetlet::CCryptTestNetlet(CNena *nena, IMessageScheduler *sched) :
	CSimpleComposedNetlet(nena, sched)
{
	className += "::CCryptTestNetlet";
	DBG_DEBUG(FMT("CCryptTestNetlet: Instantiating %1% for %2%") % CRYPT_TEST_NETLET_NAME % SIMPLE_ARCH_NAME);

	config = new netletEdit::CryptTestNetletConfig();

	// TODO instead of instantiating the BBs directly, a factory/repository should be used
	IBuildingBlock* bb;
	bb = new Bb_Frag(nena, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new Bb_Header(nena, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new Bb_Pad(nena, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new Bb_Enc(nena, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new Bb_CRC(nena, sched, this);
	buildingBlocks[bb->getId()] = bb;

	rewire();
}

/**
 * Destructor
 */
CCryptTestNetlet::~CCryptTestNetlet()
{
	DBG_DEBUG("CCryptTestNetlet: ~CCryptTestNetlet()");

	delete (netletEdit::CryptTestNetletConfig*) config;
	config = NULL;

	map<std::string, IBuildingBlock*>::const_iterator it;
	for (it = buildingBlocks.begin(); it != buildingBlocks.end(); it++) {
		delete it->second;
	}
	buildingBlocks.clear();
}

/**
 * @brief	Returns the Netlet's meta data
 */
INetletMetaData* CCryptTestNetlet::getMetaData() const
{
	return (INetletMetaData*) &cryptTestNetletMetaData;
}

/* ========================================================================= */

/**
 * Constructor
 */
CCryptTestNetletMetaData::CCryptTestNetletMetaData()
{
	MultiplexerFactories::iterator nit= multiplexerFactories.find(getArchName());

	if (nit == multiplexerFactories.end())
		DBG_ERROR(FMT("SimpleMultimediaNetlet: %1% not found => %2% cannot be loaded") % getArchName() % getId());
	else
		netletFactories[getArchName()][getId()] = (INetletMetaData*) this;
}

/**
 * Destructor
 */
CCryptTestNetletMetaData::~CCryptTestNetletMetaData()
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
std::string CCryptTestNetletMetaData::getArchName() const
{
	return SIMPLE_ARCH_NAME;
}

/**
 * Return Netlet name
 */
const std::string& CCryptTestNetletMetaData::getId() const
{
	return CRYPT_TEST_NETLET_NAME;
}

/**
 * @brief	Returns true if the Netlet does not offer any transport service
 * 			for applications, false otherwise.
 */
bool CCryptTestNetletMetaData::isControlNetlet() const
{
	return false;
}

int CCryptTestNetletMetaData::canHandle(const std::string& uri, std::string& req) const
{
	return 1;
}

/**
 * Create an instance of the Netlet
 */
INetlet* CCryptTestNetletMetaData::createNetlet(CNena *nodeA, IMessageScheduler *sched)
{
	return new CCryptTestNetlet(nodeA, sched);
}

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
