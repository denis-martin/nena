
#ifndef OMNETNETADAPT_H_
#define OMNETNETADAPT_H_

#include "messageBuffer.h"
#include "netAdapt.h"

#include "archdep/ipv4.h"

#include "omnetPacket_m.h"

#include <omnetpp.h>

#include <string>

class CSystemOmnet;

/**
 * @brief Simple OMNeT++ network access
 */
class COmnetNetAdapt : public INetAdapt
{
protected:
	CSystemOmnet *sys;
	std::string gate;				///< cSimpleModule gate to use
	int gateIndex;					///< Index of gate vector
	std::string outputGate;
	std::string inputGate;

	std::string name;				///< name of this network access
	ipv4::CLocatorValue iface_addr;	///< address associated to this network access

public:
	COmnetNetAdapt(CNena *nodeA, CSystemOmnet *sys,
			const std::string& arch, const std::string& uri,
			std::string gate, int gateIndex = -1);
	virtual ~COmnetNetAdapt();

	/**
	 * @brief	Check whether network adaptor is ready for sending/receiving data
	 */
	virtual bool isReady() const { return true; }

	// inherited from INodeArchProcessor

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

	// inherited from INetAdapt

	virtual const std::string& getName() const;

	/**
	 * @brief return a unique name string for this MessageProcessor
	 */
	virtual const std::string & getId () const;

	// new methods

	void configureInterface();
	void handleOmnetPacket(COmnetPacket *pkt);

};

#endif // OMNETNETADAPT_H_
