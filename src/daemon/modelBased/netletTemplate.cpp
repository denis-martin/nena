/*
 * netletTemplate.cpp
 *
 *  Created on: 12.06.2012
 *      Author: benjamin
 */

#include "netletTemplate.h"

#include <debug.h>

#include <vector>

#include <boost/regex.hpp>

namespace edu_kit_tm {
namespace itm {
namespace generic {

using namespace std;
using namespace boost;

CNetletMetaDataTemplate::CNetletMetaDataTemplate(string netletId, CNena *n):
	netletId(netletId),
	fControlNetlet(false),
	nena(n)
{
	if (not nena->getConfig()->hasParameter(netletId, "archName"))
		throw EMissingConfig ("archName missing for netlet " + netletId);

	nena->getConfig()->getParameter(netletId, "archName", archName);

	if (nena->getConfig()->hasParameter(netletId, "isControlNetlet", XMLFile::BOOL))
		nena->getConfig()->getParameter(netletId, "isControlNetlet", fControlNetlet);

	if (nena->getConfig()->hasParameter(netletId, "handlerRegEx"))
		nena->getConfig()->getParameter(netletId, "handlerRegEx", handlerRegEx);

	MultiplexerFactories::iterator nit= multiplexerFactories.find(getArchName());

	if (nit == multiplexerFactories.end())
		DBG_ERROR(FMT("NetletMetaDataTemplate: %1% not found => %2% cannot be loaded") % getArchName() % getId());
	else
		netletFactories[getArchName()][getId()] = (INetletMetaData*) this;
}

CNetletMetaDataTemplate::~CNetletMetaDataTemplate()
{
	NetletFactories::iterator ait = netletFactories.find(getArchName());
	if (ait != netletFactories.end()) {
		map<string, INetletMetaData*>::iterator nit= ait->second.find(getId());
		if (nit != ait->second.end()) {
			ait->second.erase(nit);
		}
	}

}

std::string CNetletMetaDataTemplate::getArchName() const
{
	return archName;
}

const std::string& CNetletMetaDataTemplate::getId() const
{
	return netletId;
}

bool CNetletMetaDataTemplate::isControlNetlet() const
{
	return fControlNetlet;
}

int CNetletMetaDataTemplate::canHandle(const std::string & uri, std::string & req) const
{
	boost::regex e(handlerRegEx);

	if (boost::regex_match(uri, e))
		return 1;

	return 0;
}

INetlet* CNetletMetaDataTemplate::createNetlet(CNena *nena, IMessageScheduler *sched)
{
	return new CNetletTemplate(nena, sched, netletId, this);
}



CNetletTemplate::CNetletTemplate(CNena *nena, IMessageScheduler *sched, std::string id, CNetletMetaDataTemplate *nmd):
	IComposableNetlet(nena, sched),
	netletMetaData(nmd)
{
	vector<vector<string> > bbChain;
	nena->getConfig()->getParameterTable(getMetaData()->getId(), "buildingBlockChain", bbChain);
	if (bbChain.empty()) {
		DBG_WARNING(FMT("%1%: no building blocks in chain!") % getMetaData()->getId());
	} else {
		for (vector<vector<string> >::const_iterator it = bbChain.begin(); it != bbChain.end(); it++) {
			config->outgoingChain.push_back((*it)[0]);
			config->incomingChain.push_front((*it)[0]);
			buildingBlocks[(*it)[0]] = nena->getRepository()->buildingBlockFactory(nena, scheduler, this, (*it)[0], (*it)[1]);
		}

		downmostBB = getBuildingBlockByName(config->incomingChain.back());
		upmostBB = getBuildingBlockByName(config->outgoingChain.back());
		rewire ();
	}
}

CNetletTemplate::~CNetletTemplate()
{

}

void CNetletTemplate::processEvent(shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled event!");
	throw EUnhandledMessage();
}

void CNetletTemplate::processTimer(shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	DBG_ERROR("Unhandled timer!");
	throw EUnhandledMessage();
}

void CNetletTemplate::processOutgoing(shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	if (msg->getFrom() == downmostBB.get()) {
		msg->setTo(next);

	} else {
		msg->setTo(upmostBB.get());
	}

	/// make sure, that our counter part at the other side gets this message
	if (!msg->hasProperty(IMessage::p_netletId))
		msg->setProperty(IMessage::p_netletId, new CStringValue(getMetaData()->getId()));
	msg->setProperty(IMessage::p_srcId, new CStringValue(nena->getNodeName()));

	msg->setFrom(this);
	sendMessage(msg);
}

void CNetletTemplate::processIncoming(shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	if (msg->getFrom() == upmostBB.get()) {
		msg->setTo(prev);

	} else {
		msg->setTo(downmostBB.get());
	}

	msg->setFrom(this);
	sendMessage(msg);
}

INetletMetaData * CNetletTemplate::getMetaData() const
{
	return netletMetaData;
}

}
}
}
