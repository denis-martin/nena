/** @file
 * dyntaTcpNetlet.h
 *
 *  Created on: Jan 11, 2012
 * @author tux
 *
 */

#ifndef DYNTATCPNETLET_H_
#define DYNTATCPNETLET_H_

#include "edu.kit.tm/itm/reliable/BasicReliableNetlet.h"

using namespace edu_kit_tm::itm::simpleArch;

namespace edu_kit_tm {
namespace itm {
namespace reliable {

const std::string& DYNTATCP_NETLET_NAME = "netlet://edu.kit.tm/itm/simpleArch/dyntaTcpNetlet";

class CDyntaTcpNetletMetaData;

/**
 * @brief	A TCP-like Netlet based on Dynamic Template Architecture
 */
class CDyntaTcpNetlet : public CBasicReliableNetlet
{
private:
	CDyntaTcpNetletMetaData* metaData;

	Bb_DataOutBasicBlock *data_sender;
	Bb_DataOutBasicBlock *segmentation;
	Bb_DataOutBasicBlock *pkt_completer_data;

	IBuildingBlock *data_receiver;
	IBuildingBlock *reassembly;
	IBuildingBlock *pkt_completer_ctrl;
	IBuildingBlock *congest_control;
	IBuildingBlock *conn_manager;
	IBuildingBlock *multiplexer;
	IBuildingBlock *ctrl_data_branch;
	IBuildingBlock *ctrl_dispatcher;
	IBuildingBlock *ctrl_receiver;
	IBuildingBlock *ctrl_sender;
	IBuildingBlock *flow_control_sender;
	IBuildingBlock *flow_control_receiver;
	IBuildingBlock *retransmitter;
	IBuildingBlock *rtt_estimator_sender;
	IBuildingBlock *send_controller;
	IBuildingBlock *receive_controller;

	std::list<Bb_DataOutBasicBlock *> outgoing_data_path;

public:
	CDyntaTcpNetlet(CDyntaTcpNetletMetaData* metaData, CNena *nena, IMessageScheduler *sched);
	virtual ~CDyntaTcpNetlet();

	std::list<Bb_DataOutBasicBlock *> getOutgoingDataPathBlocks();

	// from INetlet

	virtual void rewire(CNena *, IMessageScheduler *);

	/**
	 * @brief Process an event message directed to this message processing unit
	 *
	 * @param msg	Pointer to message
	 */
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

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

	/**
	 * @brief	Returns the Netlet's meta data
	 */
	virtual INetletMetaData* getMetaData() const;

	virtual const std::string & getId () const { return getMetaData()->getId (); }
};

/**
 * @brief Simple composed Netlet meta data
 *
 * The Netlet meta data class has two purposes: On one hand, it describes
 * the Netlet's properties, capabilities, etc., on the other hand, it provides
 * a factory functions that will be used by the node architecture daemon to
 * instantiate new Netlets of this type.
 */
class CDyntaTcpNetletMetaData : public INetletMetaData
{
private:
	std::map<std::string, std::string> properties;

public:
	CDyntaTcpNetletMetaData();
	virtual ~CDyntaTcpNetletMetaData();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;

	virtual int canHandle(const std::string& uri, std::string& req) const;

	virtual INetlet* createNetlet(CNena *nodeA, IMessageScheduler *sched); // factory function
};

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm

#endif /* DYNTATCPNETLET_H_ */
