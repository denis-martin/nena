/**@file
 * simpleVideoTransportWithFecNetlet.h
 *
 * @brief	Simple composed Netlet
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Aug 14, 2009
 *      Author: denis
 */

#include "simpleVideoTransportWithFecNetlet.h"

#include "nena.h"
#include "netAdaptBroker.h"
#include "messageBuffer.h"

#include "configs/bbcfg_SimpleVideoTransportWithFECNetlet.h"

#include <pugixml.h>
#include <sstream>

namespace edu_kit_tm {
namespace itm {
namespace simpleArch {

using namespace std;
using namespace pugi;

/**
 * Initializer for shared library. Registers factory function.
 */
static CSimpleVideoTransportWithFecNetletMetaData simpleVideoTransportWithFecNetletMetaData;

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleVideoTransportWithFecNetlet::CSimpleVideoTransportWithFecNetlet(CNena *nodeA, IMessageScheduler *sched) :
	CSimpleComposedNetlet(nodeA, sched)
{
	className += "::CSimpleVideoTransportWithFecNetlet";
	DBG_DEBUG(FMT("CSimpleVideoTransportWithFecNetlet: Instantiating %1% for %2%") % SIMPLE_VIDEOTRANSPORTWITHFEC_NETLET_NAME % SIMPLE_ARCH_NAME);

	// TODO instead of instantiating the BBs directly, a factory/repository should be used
	IBuildingBlock* bb;
	bb = new CVid_Converter(nodeArch, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new CVid_IDCT(nodeArch, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new CVid_Quantizer_HiFEC(nodeArch, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new CVid_Serializer_HiFEC(nodeArch, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new edu_kit_tm::itm::transport::Bb_SimpleSmooth(nodeArch, sched, this);
	buildingBlocks[bb->getId()] = bb;
	bb = new edu_kit_tm::itm::transport::Bb_SimpleMultiStreamer(nodeArch, sched, this);
	buildingBlocks[bb->getId()] = bb;

	config = new netletEdit::SimpleVideoTransportWithFECNetletConfig();
	config->xmlConfig.reset(new xml_document());

	string confstr = nodeA->getNetletConfig(SIMPLE_VIDEOTRANSPORTWITHFEC_NETLET_NAME, "specific");
	istringstream confin (confstr);
	xml_parse_result result;

	if (!(result = config->xmlConfig->load(confin)))
			throw EConfig (result.description());
	rewire();

//	// test
//	CBuffer* buf = new CBuffer(1228800);
//	for (int i = 0; i < buf->size(); i++) {
//		(*buf)[i] = (char) (rand() % 255);
//	}
//	*(int*) &(buf->at(0)) = 640;
//	*(int*) &(buf->at(4)) = 480;
//	CMessageBuffer* cp = new CMessageBuffer(this, this, IMessage::t_outgoing, buf);
//
//	cp->setProperty (IMessage::p_destId, new CStringValue ("bla"));
//
//	DBG_DEBUG(FMT("Sending dummy packet"));
//
//	sendMessage (cp);
}

/**
 * Destructor
 */
CSimpleVideoTransportWithFecNetlet::~CSimpleVideoTransportWithFecNetlet()
{
	DBG_DEBUG("CSimpleVideoTransportWithFecNetlet: ~CSimpleVideoTransportWithFecNetlet()");

	delete (netletEdit::SimpleVideoTransportWithFECNetletConfig*) config;
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
INetletMetaData* CSimpleVideoTransportWithFecNetlet::getMetaData() const
{
	return (INetletMetaData*) &simpleVideoTransportWithFecNetletMetaData;
}

/**
 * @brief	Set the number of FEC levels.
 *
 * @param levels	Number of FEC levels. Allowed values are 0 (no FEC),
 * 					1 (one FEC level), 2 (two FEC levels)
 */
void CSimpleVideoTransportWithFecNetlet::setFecLevels(int levels)
{
	assert(levels >= 0 && levels < 3); // [0..2]

	map<std::string, IBuildingBlock*>::iterator it;
	for (it = buildingBlocks.begin(); it != buildingBlocks.end(); it++) {
		IVid_Component* vc = dynamic_cast<IVid_Component*>(it->second);
		if (vc != NULL) {
			vc->setFecLevels(levels);
		}
	}
}

/* ========================================================================= */

/**
 * Constructor
 */
CSimpleVideoTransportWithFecNetletMetaData::CSimpleVideoTransportWithFecNetletMetaData()
{
	setProperty(p_reliabilityScore, boost::shared_ptr<CIntValue>(new CIntValue(20))); // TODO: to be removed

	MultiplexerFactories::iterator nit= multiplexerFactories.find(getArchName());

	if (nit == multiplexerFactories.end())
		DBG_ERROR(FMT("SimpleVideoTransportWithFecNetletNetlet: %1% not found => %2% cannot be loaded") % getArchName() % getId());
	else
		netletFactories[getArchName()][getId()] = (INetletMetaData*) this;

}

/**
 * Destructor
 */
CSimpleVideoTransportWithFecNetletMetaData::~CSimpleVideoTransportWithFecNetletMetaData()
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
std::string CSimpleVideoTransportWithFecNetletMetaData::getArchName() const
{
	return SIMPLE_ARCH_NAME;
}

/**
 * Return Netlet name
 */
const std::string& CSimpleVideoTransportWithFecNetletMetaData::getId() const
{
	return SIMPLE_VIDEOTRANSPORTWITHFEC_NETLET_NAME;
}

/**
 * @brief	Returns true if the Netlet does not offer any transport service
 * 			for applications, false otherwise.
 */
bool CSimpleVideoTransportWithFecNetletMetaData::isControlNetlet() const
{
	return false;
}

/**
 * @brief	Returns confidence value for the given name (uri) and the given
 * 			requirements (req). A value of 0 means, that the Netlet cannot
 * 			handle the request at all.
 */
int CSimpleVideoTransportWithFecNetletMetaData::canHandle(const std::string& uri, std::string& req) const
{
	return (uri.compare(0, 8, "video://") == 0) ? 1 : 0;
}

/**
 * Create an instance of the Netlet
 */
INetlet* CSimpleVideoTransportWithFecNetletMetaData::createNetlet(CNena *nodeA, IMessageScheduler *sched)
{
	return new CSimpleVideoTransportWithFecNetlet(nodeA, sched);
}

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
