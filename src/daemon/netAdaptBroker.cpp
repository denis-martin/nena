/**
 * netAdaptBroker.cpp
 *
 * @brief Network Access Broker (Manager)
 *
 * (c) 2008-2009 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 9, 2008
 *      Author: denis
 */

#include "netAdaptBroker.h"
#include "nena.h"

using namespace std;

const string brokerName = "internalservice://nena/netAdaptBroker";

/**
 * Constructor
 */
CNetAdaptBroker::CNetAdaptBroker(CNena* nena, IMessageScheduler *sched) :
		IMessageProcessor(sched), na(nena)
{
	className += "::CNetAdaptBroker";
	setId(brokerName); // TODO: to be fixed with brokerName
	na->registerInternalService(getId(), this);
}

/**
 * Destructor
 */
CNetAdaptBroker::~CNetAdaptBroker()
{
	// TODO Auto-generated destructor stub
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CNetAdaptBroker::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CNetAdaptBroker::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CNetAdaptBroker::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CNetAdaptBroker::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

/**
 * @brief Register a network adaptor
 *
 * @param archName	Name of architecture spoken on the network the adaptor connects to
 * @param netAdapt	Pointer to INetAdapt
 */
void CNetAdaptBroker::registerNetAdapt(INetAdapt *netAdapt)
{
	string archName;
	try {
		archName = netAdapt->getProperty<CStringValue>(INetAdapt::p_archid)->value();
	} catch (exception e) {
		DBG_FAIL("NetAdapt with missing ArchID");

	}
	netAdapts[archName].push_back(netAdapt);

	DBG_DEBUG(FMT("Registered NA %1% for %2%") % netAdapt->getId() % archName);
	try {
		DBG_INFO(FMT("  p_name = %1%") % netAdapt->getProperty<CStringValue>(INetAdapt::p_name)->value());
	} catch (std::exception& e) {
	}

	DBG_INFO(FMT("  p_archid = %1%") % archName);

	try {
		DBG_INFO(FMT("  p_addr = %1%") % netAdapt->getProperty<CStringValue>(INetAdapt::p_addr)->value());
	} catch (std::exception& e) {
	}

	try {
		DBG_INFO(FMT("  p_up = %1%") % netAdapt->getProperty<CBoolValue>(INetAdapt::p_up)->value());
	} catch (std::exception& e) {
	}

	try {
		DBG_INFO(FMT("  p_linkencap = %1%") % netAdapt->getProperty<CStringValue>(INetAdapt::p_linkencap)->value());
	} catch (std::exception& e) {
	}

	try {
		DBG_INFO(FMT("  p_bandwidth = %1%") % (unsigned int) netAdapt->getProperty<CIntValue>(INetAdapt::p_bandwidth)->value());
	} catch (std::exception& e) {
	}

	try {
		DBG_INFO(FMT("  p_broadcast = %1%") % netAdapt->getProperty<CBoolValue>(INetAdapt::p_broadcast)->value());
	} catch (std::exception& e) {
	}

	try {
		DBG_INFO(FMT("  p_duplex = %1%") % netAdapt->getProperty<CBoolValue>(INetAdapt::p_duplex)->value());
	} catch (std::exception& e) {
	}

	try {
		DBG_INFO(FMT("  p_mtu = %1%") % (unsigned int) netAdapt->getProperty<CIntValue>(INetAdapt::p_mtu)->value());
	} catch (std::exception& e) {
	}
}

/**
 * @brief Return a list network adaptors fitting to the given architecture
 *
 * @param archName	Name of architecture
 *
 * @returns List of INetAdapt pointers
 */
std::list<INetAdapt *>& CNetAdaptBroker::getNetAdapts(std::string archName)
{
	return netAdapts[archName];
}

/**
 * @brief Return a network adaptor fitting to the given name
 *
 * @param netletName	Name of the NetAdapt
 *
 * @returns INetAdapt pointer
 */
INetAdapt* CNetAdaptBroker::getNetAdapt(std::string netadaptName)
{
	std::map<std::string, std::list<INetAdapt *> >::iterator mit;
	for ( mit=netAdapts.begin(); mit != netAdapts.end(); mit++ ) {
		list<INetAdapt *>::iterator lit;
		for ( lit=mit->second.begin(); lit != mit->second.end(); lit++ ) {
			INetAdapt* n = *lit;
			if (n->getId() == netadaptName) {
				return n;
			}
		}
	}
	return NULL;
}
