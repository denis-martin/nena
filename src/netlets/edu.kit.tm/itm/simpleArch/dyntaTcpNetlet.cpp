/** @file
 * dyntaTcpNetlet.cpp
 *
 *  Created on: Jan 11, 2012
 * @author Eduard Frank
 *
 */

#include <pugixml.h>
#include <sstream>

#include "dyntaTcpNetlet.h"
#include "nena.h"
#include "netAdaptBroker.h"
#include "messageBuffer.h"

#include "edu.kit.tm/itm/reliable/cong/bb_RenoCongestionControl.h"
#include "edu.kit.tm/itm/reliable/cong/bb_NewRenoCongestionControl.h"
#include "edu.kit.tm/itm/reliable/cong/bb_TahoeCongestionControl.h"
#include "edu.kit.tm/itm/reliable/cong/bb_ScalableTCPCongestionControl.h"

#include "edu.kit.tm/itm/reliable/retransmit/bb_RetransmitterPoll.h"
#include "edu.kit.tm/itm/reliable/retransmit/bb_RetransmitterSimple.h"
#include "edu.kit.tm/itm/reliable/retransmit/bb_RetransmitterSACK.h"

#include "edu.kit.tm/itm/reliable/flow/bb_FlowControlSenderWrap.h"
#include "edu.kit.tm/itm/reliable/flow/bb_FlowControlReceiverWrap.h"
#include "edu.kit.tm/itm/reliable/flow/bb_FlowControlSenderPeriodic.h"
#include "edu.kit.tm/itm/reliable/flow/bb_FlowControlReceiverPeriodic.h"

#include "edu.kit.tm/itm/reliable/segment/bb_SegmentationDatagram.h"
#include "edu.kit.tm/itm/reliable/segment/bb_SegmentationStream.h"

#include "edu.kit.tm/itm/reliable/reassembly/bb_ReassemblyDatagram.h"
#include "edu.kit.tm/itm/reliable/reassembly/bb_ReassemblyStream.h"

#include "edu.kit.tm/itm/reliable/sendcontrol/bb_SendControllerSimple.h"

#include "edu.kit.tm/itm/reliable/rtt/bb_RTTEstimatorJacobson.h"

#include "edu.kit.tm/itm/reliable/datasend/bb_DataSenderSimple.h"
#include "edu.kit.tm/itm/reliable/datasend/bb_DataSenderEvent.h"
#include "edu.kit.tm/itm/reliable/datasend/bb_DataSenderEventBulk.h"
#include "edu.kit.tm/itm/reliable/datasend/bb_DataSenderInet.h"

#include "edu.kit.tm/itm/reliable/datareceive/bb_DataReceiverSimple.h"

#include "edu.kit.tm/itm/reliable/receivecontrol/bb_ReceiveControllerSimple.h"
#include "edu.kit.tm/itm/reliable/receivecontrol/bb_ReceiveControllerInterval.h"
#include "edu.kit.tm/itm/reliable/receivecontrol/bb_ReceiveControllerDelay.h"
#include "edu.kit.tm/itm/reliable/receivecontrol/bb_ReceiveControllerSACK.h"

#include "edu.kit.tm/itm/reliable/bucket/bb_TokenBucketSender.h"

#include "edu.kit.tm/itm/reliable/conn/bb_ConnectionManagerSimple.h"

#include "edu.kit.tm/itm/reliable/bb_PacketCompleter.h"
#include "edu.kit.tm/itm/reliable/bb_ReceiveController.h"
#include "edu.kit.tm/itm/reliable/bb_DataSender.h"
#include "edu.kit.tm/itm/reliable/bb_DataReceiver.h"
#include "edu.kit.tm/itm/reliable/bb_ConnectionManager.h"
#include "edu.kit.tm/itm/reliable/bb_ControlDataBrancher.h"
#include "edu.kit.tm/itm/reliable/bb_ControlDispatcher.h"
#include "edu.kit.tm/itm/reliable/bb_ControlReceiver.h"
#include "edu.kit.tm/itm/reliable/bb_ControlSender.h"
#include "edu.kit.tm/itm/reliable/bb_Dropper.h"
#include "edu.kit.tm/itm/reliable/bb_FlowControlSender.h"
#include "edu.kit.tm/itm/reliable/bb_FlowControlReceiver.h"
#include "edu.kit.tm/itm/reliable/bb_Retransmitter.h"
#include "edu.kit.tm/itm/reliable/bb_RTTEstimatorSender.h"
#include "edu.kit.tm/itm/reliable/bb_Segmentation.h"
#include "edu.kit.tm/itm/reliable/bb_SendController.h"
#include "edu.kit.tm/itm/reliable/bb_BucketSender.h"
#include "edu.kit.tm/itm/reliable/bb_BucketReceiver.h"
#include "edu.kit.tm/itm/reliable/bb_Multiplexer.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace edu_kit_tm::itm::simpleArch;

namespace edu_kit_tm {
namespace itm {
namespace reliable {

using namespace std;
using namespace pugi;
using namespace boost::property_tree;

/**
 * Initializer for shared library. Registers factory function.
 */
static CDyntaTcpNetletMetaData dyntaTcpNetletMetaData;

/* ========================================================================= */

/**
 * Constructor
 */
CDyntaTcpNetlet::CDyntaTcpNetlet(CDyntaTcpNetletMetaData* metaData, CNena *nena, IMessageScheduler *sched) :
	CBasicReliableNetlet(nena, sched), metaData(metaData)
{
	className += "::CDyntaTcpNetlet";
	setId(DYNTATCP_NETLET_NAME);
	DBG_DEBUG(FMT("%1% instantiated for %2%") % getId() % getMetaData()->getArchName());

	rewire(nena, sched);
}

void CDyntaTcpNetlet::rewire(CNena *nodeA, IMessageScheduler *sched)
{
	pkt_completer_ctrl = new Bb_PacketCompleter(nodeA, PacketHeader::CONTROL_TYPE, sched, this);
	pkt_completer_data = new Bb_PacketCompleter(nodeA, PacketHeader::DATA_TYPE, sched, this);

	retransmitter = new Bb_RetransmitterSimple(nodeA, sched, this);
	receive_controller = new Bb_ReceiveControllerInterval(nodeA, sched, this);

	data_sender = new Bb_DataSenderEventBulk(nodeA, sched, this);
	data_receiver = new Bb_DataReceiverSimple(nodeA, sched, this);
	ctrl_receiver = new Bb_ControlReceiver(nodeA, sched, this);
	ctrl_sender = new Bb_ControlSender(nodeA, sched, this);

	rtt_estimator_sender = new Bb_RTTEstimatorJacobson(nodeA, sched, this);
	segmentation = new Bb_SegmentationStream(nodeA, sched, this);
	reassembly = new Bb_ReassemblyStream(nodeA, sched, this);
	send_controller = new Bb_SendControllerSimple(nodeA, sched, this);

	conn_manager = new Bb_ConnectionManagerSimple(nodeA, sched, this);
	multiplexer  = new Bb_Multiplexer(nodeA, sched, this);

	ctrl_data_branch = new Bb_ControlDataBrancher(nodeA, sched, this, data_receiver, ctrl_receiver);

	ctrl_dispatcher = new Bb_ControlDispatcher(nodeA, sched, this);

	flow_control_sender = new Bb_FlowControlSenderWrap(nodeA, sched, this, ctrl_dispatcher);
	congest_control = new Bb_NewRenoCongestionControl(nodeA, sched, this, ctrl_dispatcher);
	flow_control_receiver = new Bb_FlowControlReceiverWrap(nodeA, sched, this);

	{
		Bb_ControlDispatcher *tmp = (Bb_ControlDispatcher *)ctrl_dispatcher;
		tmp->setACKBranch(retransmitter);
		tmp->setNACKBranch(retransmitter);
		tmp->setConnBranch(conn_manager);
		tmp->setFlowRequestBranch(flow_control_receiver);
		tmp->setFlowStatusBranch(flow_control_sender);

		tmp->setCongStatusBranch(congest_control);
		tmp->setRTTStatusBranch(rtt_estimator_sender);
	}

	buildingBlocks[data_sender->getId()] = data_sender;
	buildingBlocks[data_receiver->getId()] = data_receiver;
	buildingBlocks[congest_control->getId()] = congest_control;
	buildingBlocks[conn_manager->getId()] = conn_manager;
	buildingBlocks[ctrl_data_branch->getId()] = ctrl_data_branch;
	buildingBlocks[ctrl_dispatcher->getId()] = ctrl_dispatcher;
	buildingBlocks[ctrl_receiver->getId()] = ctrl_receiver;
	buildingBlocks[ctrl_sender->getId()] = ctrl_sender;
	buildingBlocks[flow_control_sender->getId()] = flow_control_sender;
	buildingBlocks[flow_control_receiver->getId()] = flow_control_receiver;
	buildingBlocks[retransmitter->getId()] = retransmitter;
	buildingBlocks[rtt_estimator_sender->getId()] = rtt_estimator_sender;
	buildingBlocks[segmentation->getId()] = segmentation;
	buildingBlocks[reassembly->getId()] = reassembly;
	buildingBlocks[send_controller->getId()] = send_controller;
	buildingBlocks[receive_controller->getId()] = receive_controller;
	buildingBlocks[pkt_completer_ctrl->getId()] = pkt_completer_ctrl;
	buildingBlocks[pkt_completer_data->getId()] = pkt_completer_data;

	// outgoing data path
	multiplexer->setNext(segmentation);
	segmentation->setNext(data_sender);
	data_sender->setNext(pkt_completer_data);
	pkt_completer_data->setNext(this);

	outgoing_data_path.push_back(segmentation);
	outgoing_data_path.push_back(data_sender);
	outgoing_data_path.push_back(pkt_completer_data);

	// outgoing control path
	receive_controller->setNext(flow_control_receiver);
	flow_control_receiver->setNext(ctrl_sender);
	flow_control_sender->setNext(ctrl_sender);
	ctrl_sender->setNext(pkt_completer_ctrl);
	pkt_completer_ctrl->setNext(this);
	conn_manager->setNext(ctrl_sender);
	send_controller->setNext(ctrl_sender);

	// incoming data
	multiplexer->setPrev(ctrl_data_branch);
	data_receiver->setPrev(reassembly);
	reassembly->setPrev(this);

	// incoming control
	ctrl_receiver->setPrev(ctrl_dispatcher);
	retransmitter->setPrev(rtt_estimator_sender);
	rtt_estimator_sender->setPrev(congest_control);
	congest_control->setPrev(send_controller);

	// Events
	data_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_DATA, retransmitter);
	data_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_DATA, rtt_estimator_sender);
	data_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_DATA, congest_control);
	data_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_FLIGHT_SIZE, congest_control);

	rtt_estimator_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_RTO, retransmitter);
	rtt_estimator_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_RTT, congest_control);

	data_receiver->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_FLOW_WND, flow_control_receiver);
	data_receiver->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_DATA_ARRIVED, receive_controller);

	send_controller->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_SEND_CREDIT, data_sender);
	send_controller->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_ACK, data_sender);

	retransmitter->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_PACKET_TIMEOUT, congest_control);
	retransmitter->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_PACKET_TIMEOUT, rtt_estimator_sender);
	retransmitter->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_RETRANSMIT, data_sender);
	retransmitter->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_FAST_RETRANSMIT, data_sender);

	congest_control->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_CONGEST_WND, send_controller);
	congest_control->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_CONGEST_RATE, send_controller);
	congest_control->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_FAST_RETRANSMIT, retransmitter);
	congest_control->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_RESET_RTT, retransmitter);
	congest_control->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_PACKET_TIMEOUT_ACK, retransmitter);

	flow_control_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_FLOW_WND, send_controller);
	flow_control_sender->registerListener(EVENT_SIMPLEARCH_RELIABLE_ON_SEND_FLOW_RATE, send_controller);

	conn_manager->registerListener(EVENT_SIMPLEARCH_RELIABLE_OPEN_CONNECTION, multiplexer);
	conn_manager->registerListener(EVENT_SIMPLEARCH_RELIABLE_PUBLISH_CONNECTION, multiplexer);

	multiplexer->registerListener(EVENT_SIMPLEARCH_RELIABLE_START_SENDING, conn_manager);
	multiplexer->registerListener(EVENT_SIMPLEARCH_RELIABLE_OPEN_CONNECTION_ACK, conn_manager);
	multiplexer->registerListener(EVENT_SIMPLEARCH_RELIABLE_PUBLISH_CONNECTION_ACK, conn_manager);

//	netletSelector->registerListener(EVENT_NETLETSELECTOR_CONNECT, conn_manager);
//	netletSelector->registerListener(EVENT_NETLETSELECTOR_BIND, conn_manager);

	conn_manager->registerListener(EVENT_SIMPLEARCH_RELIABLE_SET_MULTIPLEXER_STATE, multiplexer);

	for(map<std::string, IBuildingBlock*>::const_iterator it = buildingBlocks.begin(); it != buildingBlocks.end(); ++it) {
		if(it->second == conn_manager || it->second == multiplexer) continue;

		conn_manager->registerListener(EVENT_SIMPLEARCH_RELIABLE_INIT_BB,  it->second);
		conn_manager->registerListener(EVENT_SIMPLEARCH_RELIABLE_START_BB, it->second);
	}
}

std::list<Bb_DataOutBasicBlock *> CDyntaTcpNetlet::getOutgoingDataPathBlocks()
{
	return outgoing_data_path;
}

/**
 * Destructor
 */
CDyntaTcpNetlet::~CDyntaTcpNetlet()
{
	DBG_DEBUG("CDyntaTcpNetlet: ~CDyntaTcpNetlet()");

	map<std::string, IBuildingBlock*>::const_iterator it;
	for (it = buildingBlocks.begin(); it != buildingBlocks.end(); it++)
		delete it->second;

	buildingBlocks.clear();
}

/**
 * @brief	Returns the Netlet's meta data
 */
INetletMetaData* CDyntaTcpNetlet::getMetaData() const
{
	return (INetletMetaData*) metaData;
}

/* ========================================================================= */

/**
 * Constructor
 */
CDyntaTcpNetletMetaData::CDyntaTcpNetletMetaData()
{
	MultiplexerFactories::iterator nit= multiplexerFactories.find(getArchName());

	if (nit == multiplexerFactories.end())
		DBG_ERROR(FMT("DyntaTcpNetlet: %1% not found => %2% cannot be loaded") % getArchName() % getId());
	else
		netletFactories[getArchName()][getId()] = (INetletMetaData*) this;
}

/**
 * Destructor
 */
CDyntaTcpNetletMetaData::~CDyntaTcpNetletMetaData()
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
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CDyntaTcpNetlet::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	boost::shared_ptr<IEvent> event = msg->cast<IEvent>();
	assert(event.get() != NULL);

	if (event->getId() == EVENT_NETLETSELECTOR_APPCONNECT) {
		event->setFrom(this);
		event->setTo(conn_manager);
		sendMessage(event);

	} else {
		throw EUnhandledMessage();

	}
}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CDyntaTcpNetlet::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if (pkt.get() == NULL) {
		DBG_ERROR("Unhandled outgoing message!");
		throw EUnhandledMessage();
	}

	// check whether the packet already traveled through one of sour building blocks
	if (pkt->getFrom() == pkt_completer_ctrl || pkt->getFrom() == pkt_completer_data) {
		// yes, so hand it off towards the network

		// make sure, that our counter part at the other side gets this message
		if (!pkt->hasProperty(IMessage::p_netletId))
			pkt->setProperty(IMessage::p_netletId, new CStringValue(getMetaData()->getId()));
		pkt->setProperty(IMessage::p_srcId, new CStringValue(nena->getNodeName()));

		pkt->setFrom(this);
		pkt->setTo(next);
		sendMessage(pkt);

	} else {
		// nope, so reset its loop trace and give it to the first BB
		pkt->flushVisitedProcessors();
		pkt->setFrom(this);
		pkt->setTo(multiplexer);
		sendMessage(pkt);
	}
}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CDyntaTcpNetlet::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	// dynamic cast only used to detect bugs early
	boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
	if(pkt.get() == NULL) {
		DBG_ERROR("Unhandled incoming message!");
		throw EUnhandledMessage();
	}

	// check whether the packet already traveled through our building blocks
	if (pkt->getFrom() == reassembly) {
		// yes
		assert(pkt->getFlowState() != NULL);

		pkt->setFrom(this);
		pkt->setTo(prev);
		sendMessage(pkt);

	} else {
		// nope, so reset its loop trace and give it to the first BB

//		DBG_INFO("Handing packet to first BB");
		pkt->flushVisitedProcessors();
		pkt->setFrom(this);
		pkt->setTo(multiplexer);
		sendMessage(pkt);
	}
}

/**
 * Return Netlet name
 */
std::string CDyntaTcpNetletMetaData::getArchName() const
{
	return SIMPLE_ARCH_NAME;
}

/**
 * Return Netlet name
 */
const std::string& CDyntaTcpNetletMetaData::getId() const
{
	return DYNTATCP_NETLET_NAME;
}

/**
 * @brief	Returns true if the Netlet does not offer any transport service
 * 			for applications, false otherwise.
 */
bool CDyntaTcpNetletMetaData::isControlNetlet() const
{
	return false;
}

/**
 * Create an instance of the Netlet
 */
INetlet* CDyntaTcpNetletMetaData::createNetlet(CNena *nodeA, IMessageScheduler *sched)
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

	return new CDyntaTcpNetlet(this, nodeA, sched);
}

int CDyntaTcpNetletMetaData::canHandle(const std::string& uri, std::string& req) const
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

} // namespace simpleArch
} // namespace itm
} // namespace edu_kit_tm
