/** @file
 * management.h
 *
 * @brief "Architect" of the Node - implementation
 *
 * (c) 2008-2010 Institut fuer Telematik, Universitaet Karlsruhe (TH), Germany
 *
 *  Created on: Dec 12, 2010
 *      Author: Benjamin Behringer
 */

#include "management.h"
#include <pugixml.h>

#include <stdio.h>
#include <signal.h>

#include <list>
#include <sstream>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <appConnector.h>
#include <debug.h>
#include "localRepository.h"

#define NENA_MANAGEMENT_ID	"internalservice://management"

using namespace pugi;
using namespace std;

using boost::scoped_ptr;
using boost::shared_ptr;
using boost::shared_mutex;
using boost::shared_lock;
using boost::unique_lock;
using namespace boost::property_tree;

/*
 * TODO: denis: is this private implementation really necessary?
 */
class Management::Private
{
public:

	CNena * nodearch;
	ISystemWrapper * sys;						///< Pointer to system class
	scoped_ptr<CNetletSelector> netletSelector;	///< Pointer to netlet selector
	scoped_ptr<INetletRepository> rep;			///< Pointer to repository
	scoped_ptr<CNetAdaptBroker> naBroker;		///< Pointer to network access broker
	list<shared_ptr<IAppServer> > appServers;	///< List of App Servers

//	void handleShow (pugi::xml_document & xmlmsg);
};

Management::Management(ISystemWrapper * s, CNena * na) :
		IMessageProcessor(NULL)
{
	className += "::Management";
	setId(NENA_MANAGEMENT_ID);

	d.reset(new Private());
	d_func()->nodearch = na;
	d_func()->sys = s;

	d_func()->nodearch->registerInternalService(getId(), this);
}

Management::~Management()
{
}

void Management::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<IEvent> ev = msg->cast<IEvent>();
	if (ev->getId() != EVENT_NETLETSELECTOR_APPCONNECT) {
		throw EUnhandledMessage();
	}

	shared_ptr<CNetletSelector::Event_AppConnect> event = msg->cast<CNetletSelector::Event_AppConnect>();

	string uri = event->getFlowState()->getRemoteId();
	IAppConnector::method_t m = event->method;

	if (m == IAppConnector::method_put) {
		// handled in processOutgoing since we need the data
		return;
	}

	DBG_DEBUG(FMT("%1%: got management request for %2% (method %3%)") % getId() % uri % m);

	// sillberg:// added temporarily until nena:// is available
	if ((uri == "nena://localhost/activeArchitectures") && (m == IAppConnector::method_get)) {

		string reply = "{ \"test\": \"test123\" }";

		shared_ptr<CMessageBuffer> reply_msg(new CMessageBuffer(buffer_t(reply.c_str())));
		reply_msg->setFrom(this);
		reply_msg->setTo(msg->getFrom());
		reply_msg->setType(IMessage::t_incoming);
		reply_msg->setFlowState(msg->getFlowState());
		sendMessage(reply_msg);

	} else if ((uri == "nena://localhost/writeTest") && (m == IAppConnector::method_put)) {
		boost::shared_ptr<CMessageBuffer> pkt = msg->cast<CMessageBuffer>();
		if (pkt.get() == NULL) {
			DBG_ERROR("Unhandled outgoing message!");
			throw EUnhandledMessage();
		}

		char cs[pkt->size()+1];
		pkt->getBuffer().read((boctet_t*) cs);
		cs[pkt->size()] = 0;

		string s(cs);
		DBG_DEBUG(FMT("%1% writeTest: %2%") % getId() %s);

	} else if ((uri.compare(0, 26,"nena://localhost/netadapt/") == 0) && (m == IAppConnector::method_get)) {
		// query for a netadapt

		// extract netadapt name
		string rawName = uri.substr(26,sizeof(uri)-26);
		string naName = "netadapt://" + rawName;
		INetAdapt* na = d_func()->naBroker->getNetAdapt(naName);

		if (na != NULL) {
			int rxRate = na->getProperty<CIntValue>(INetAdapt::p_rx_rate)->value();
			int txRate = na->getProperty<CIntValue>(INetAdapt::p_tx_rate)->value();
			int maxRate = na->getProperty<CIntValue>(INetAdapt::p_bandwidth)->value() / 8; // in bytes per second
			int freeRate = na->getProperty<CIntValue>(INetAdapt::p_freeBandwidth)->value() / 8; // in bytes per second

			// quick and dirty: determine carrier state
			char carrier = '0'; // be pessimistic
			try {
				ifstream ifcarrier((FMT("/sys/class/net/%1%/carrier") % rawName).str().c_str());
				ifcarrier.get(carrier);
			} catch(...) {
				// nothing
			}

			string reply = (FMT("{ \"maximumBps\": %1%, \"freeBps\": %2%, \"rxRate\": %3%, \"txRate\": %4%, \"carrier\": %5%}") %
					maxRate % freeRate % rxRate % txRate % carrier).str();

			shared_ptr<CMessageBuffer> reply_msg(new CMessageBuffer(buffer_t(reply.c_str())));
			reply_msg->setFrom(this);
			reply_msg->setTo(msg->getFrom());
			reply_msg->setType(IMessage::t_incoming);
			reply_msg->setFlowState(msg->getFlowState());
			sendMessage(reply_msg);

			DBG_DEBUG(FMT("%1%: netadapt query for %2%, sending\n%3%") % getId() % naName % reply);

		} else {
			DBG_ERROR(FMT("%1%: %2% not found") % getId() % naName);

		}

	} else if ((uri == "nena://localhost/stateViewer/architectures") && (m == IAppConnector::method_get)) {
		// QnD for stateViewer
		string reply("{ \"architectures\": [");

		list<INetletMultiplexer *>::const_iterator it;
		for (it = d_func()->rep->multiplexers.begin(); it != d_func()->rep->multiplexers.end(); it++)
			reply += " \"" + (*it)->getMetaData()->getArchName() + "\",";

		if (!(d_func()->rep->multiplexers.empty()))
			reply.erase(reply.size()-1, 1);

		reply += " ] }";

		shared_ptr<CMessageBuffer> reply_msg(new CMessageBuffer(buffer_t(reply.c_str())));
		reply_msg->setFrom(this);
		reply_msg->setTo(msg->getFrom());
		reply_msg->setType(IMessage::t_incoming);
		reply_msg->setFlowState(msg->getFlowState());
		sendMessage(reply_msg);

	} else if ((uri == "nena://localhost/stateViewer/netlets") && (m == IAppConnector::method_get)) {
		// QnD for stateViewer
		string reply("{ \"netlets\": [");

		list<INetlet *>::const_iterator it;
		for (it = d_func()->rep->netlets.begin(); it != d_func()->rep->netlets.end(); it++) {
			reply += " { \"name\": \"" + (*it)->getId() + "\",";
			reply += " \"arch\": \"" + (*it)->getMetaData()->getArchName() + "\" },";
		}

		if (!(d_func()->rep->netlets.empty()))
			reply.erase(reply.size()-1, 1);

		reply += " ] }";

		shared_ptr<CMessageBuffer> reply_msg(new CMessageBuffer(buffer_t(reply.c_str())));
		reply_msg->setFrom(this);
		reply_msg->setTo(msg->getFrom());
		reply_msg->setType(IMessage::t_incoming);
		reply_msg->setFlowState(msg->getFlowState());
		sendMessage(reply_msg);

	} else if ((uri == "nena://localhost/stateViewer/netAdapts") && (m == IAppConnector::method_get)) {
		// QnD for stateViewer
		string reply("{ \"netAdapts\": [");

		bool hasNetAdapts = false;
		list<INetletMultiplexer *>::const_iterator it;
		for (it = d_func()->rep->multiplexers.begin(); it != d_func()->rep->multiplexers.end(); it++) {
			string arch = (*it)->getMetaData()->getArchName();
			list<INetAdapt *>& nas = d_func()->naBroker->getNetAdapts(arch);

			list<INetAdapt *>::const_iterator nit;
			for (nit = nas.begin(); nit != nas.end(); nit++) {
				reply += " { \"name\": \"" + (*nit)->getId() + "\",";
				reply += " \"arch\": \"" + arch + "\" },";
				if (!hasNetAdapts) hasNetAdapts = true;
			}

		}

		if (hasNetAdapts)
			reply.erase(reply.size()-1, 1);

		reply += " ] }";

		shared_ptr<CMessageBuffer> reply_msg(new CMessageBuffer(buffer_t(reply.c_str())));
		reply_msg->setFrom(this);
		reply_msg->setTo(msg->getFrom());
		reply_msg->setType(IMessage::t_incoming);
		reply_msg->setFlowState(msg->getFlowState());
		sendMessage(reply_msg);

	} else if ((uri == "nena://localhost/stateViewer/appFlowStates") && (m == IAppConnector::method_get)) {
		// QnD for stateViewer
		string reply("{ \"appFlowStates\": [");

		list<IAppConnector*> apps;
		d_func()->netletSelector->getActiveApps(apps);
		list<IAppConnector*>::const_iterator it;
		for (it = apps.begin(); it != apps.end(); it++) {
			shared_ptr<CFlowState> flowState = (*it)->getFlowState();
			reply += " { \"name\": \"" + (*it)->getIdentifier() + "\",";
			reply += (FMT(" \"id\": %1%") % flowState->getFlowId()).str() + ",";
			reply += " \"remoteUri\": \"" + (*it)->getRemoteURI() + "\",";
			reply += " \"netlet\": \"" + (*it)->getNetlet()->getId() + "\",";
			reply += " \"arch\": \"" + (*it)->getNetlet()->getMetaData()->getArchName() + "\",";
			reply += (FMT(" \"method\": %1%") % (*it)->getMethod()).str() + ",";
			reply += (FMT(" \"tx_floating\": %1%") % flowState->getOutFloatingPackets()).str() + ",";
			reply += (FMT(" \"rx_floating\": %1%") % flowState->getInFloatingPackets()).str() + ",";

			shared_ptr<CFlowState::StatisticsObject> sso = flowState->getStateObject(FLOWSTATE_STATISTICS_ID)->cast<CFlowState::StatisticsObject>();
			uint32_t packetCountIn = sso->values[CFlowState::stat_packetCountIn]->cast<CIntValue>()->value();
			uint32_t lossCountIn = sso->values[CFlowState::stat_lossCountIn]->cast<CIntValue>()->value();
			uint32_t rPacketCountIn = sso->values[CFlowState::stat_remote_packetCountIn]->cast<CIntValue>()->value();
			uint32_t rLossCountIn = sso->values[CFlowState::stat_remote_lossCountIn]->cast<CIntValue>()->value();
			double rLossRate = sso->values[CFlowState::stat_remote_lossRate]->cast<CDoubleValue>()->value();

			reply += (FMT(" \"stat_packetCountIn\": %1%") % packetCountIn).str() + ",";
			reply += (FMT(" \"stat_lossCountIn\": %1%") % lossCountIn).str() + ",";
			reply += (FMT(" \"stat_remote_packetCountIn\": %1%") % rPacketCountIn).str() + ",";
			reply += (FMT(" \"stat_remote_lossCountIn\": %1%") % rLossCountIn).str() + ",";
			reply += (FMT(" \"stat_remote_curLossRatio\": %1%") % rLossRate).str() + ",";

			double ratio = 0;
			if (lossCountIn + packetCountIn > 0)
				ratio = (double) lossCountIn / (lossCountIn + packetCountIn);

			double rRatio = 0;
			if (lossCountIn + packetCountIn > 0)
				rRatio = (double) rLossCountIn / (rLossCountIn + rPacketCountIn);

			reply += (FMT(" \"stat_lossRatio\": %1%") % ratio).str() + ",";
			reply += (FMT(" \"stat_remote_lossRatio\": %1%") % rRatio).str() + " },";

		}

		if (!apps.empty())
			reply.erase(reply.size()-1, 1);

		reply += " ] }";

		shared_ptr<CMessageBuffer> reply_msg(new CMessageBuffer(buffer_t(reply.c_str())));
		reply_msg->setFrom(this);
		reply_msg->setTo(msg->getFrom());
		reply_msg->setType(IMessage::t_incoming);
		reply_msg->setFlowState(msg->getFlowState());
		sendMessage(reply_msg);

	// sillberg://
	} else if ((uri == "nena://localhost/sillberg/capabilities") && (m == IAppConnector::method_get)) {
		string cap_template = "{ "
				"\"path\": \"%1%\", "
				"\"capacity\": %2%, "
				"\"free\": %3%, "
				"\"priority\": %4%, " // ???
				"\"bytesOut\": %5%, "
				"\"carrier\": %6%"
			"}";

		string reply = "{ \"capabilities\" : [ ";

		list<INetAdapt*> nas = d_func()->naBroker->getNetAdapts("architecture://edu.kit.tm/itm/simpleArch");
		list<INetAdapt*>::const_iterator it;
		for (it = nas.begin(); it != nas.end(); it++) {
			string path = (*it)->getProperty<CStringValue>(INetAdapt::p_name)->value();
			size_t capacity = (*it)->getProperty<CIntValue>(INetAdapt::p_bandwidth)->value(); // bit/s
			size_t freeCap = (*it)->getProperty<CIntValue>(INetAdapt::p_freeBandwidth)->value(); // bit/s
			size_t used = (*it)->getProperty<CIntValue>(INetAdapt::p_tx_rate)->value(); // bytes/s
			int priority = 0;
			string rawName = path.substr(11, sizeof(path)-11);

			// quick and dirty: determine carrier state
			char carrier = '0'; // be pessimistic
			string filename = (FMT("/sys/class/net/%1%/carrier") % rawName).str();
			try {
				ifstream ifcarrier(filename.c_str());
				ifcarrier.get(carrier);
			} catch(exception& e) {
				DBG_DEBUG(FMT("%1%: error reading %2%: %3%") % getId() % filename % e.what());
				// nothing else
			}

			// insert comma?
			if (it != nas.begin())
				reply.append(", ");

			reply.append((FMT(cap_template) % path % capacity % freeCap % priority % used % carrier).str());

		}

		reply.append(" ] }");

		shared_ptr<CMessageBuffer> reply_msg(new CMessageBuffer(buffer_t(reply.c_str())));
		reply_msg->setFrom(this);
		reply_msg->setTo(msg->getFrom());
		reply_msg->setType(IMessage::t_incoming);
		reply_msg->setFlowState(msg->getFlowState());
		sendMessage(reply_msg);

	} else {
		throw EUnhandledMessage();
	}
}

void Management::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

void Management::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	shared_ptr<CMessageBuffer> mbuf = msg->cast<CMessageBuffer>();
	if (mbuf.get() == NULL)
		throw EUnhandledMessage("processOutgoing(): not a CMessageBuffer");

	bool endOfStream = false;
	try {
		endOfStream = mbuf->getProperty<CBoolValue>(IMessage::p_endOfStream)->value();
	} catch (...) {
		// nothing
	}

	if (endOfStream)
		return;

	string uri = mbuf->getFlowState()->getRemoteId();
	IAppConnector::method_t m = mbuf->getFlowState()->getMethod();

	DBG_DEBUG(FMT("%1%: got management request for %2% (method %3%)") % getId() % uri % m);

	if ((uri.compare(0, 26,"nena://localhost/netadapt/") == 0) && (m == IAppConnector::method_put)) {
		// query for a netadapt

		// extract netadapt name
		std::string naName = uri.substr(26,sizeof(uri)-26);
		naName = "netadapt://" + naName;
		INetAdapt* na = d_func()->naBroker->getNetAdapt(naName);

		if (na != NULL) {
			if (mbuf->size() > 0) {
				char* cs = new char[mbuf->size()+1];
				mbuf->getBuffer().read((boctet_t*) cs);
				cs[mbuf->size()] = 0;

				DBG_DEBUG(FMT("%1%: netadapt settings for %2%, data:\n%3%") % getId() % naName % string(cs));

				ptree pt;
				stringstream ss(cs);
				read_json(ss, pt);
				delete[] cs;
				cs = NULL;

				int maxBps = pt.get("maximumBps", -1); // bytes per second
				if (maxBps > 0) {
					DBG_DEBUG(FMT("%1%: setting %2% maximumBps to %3%") % getId() % naName % maxBps);
					na->getProperty<CIntValue>(INetAdapt::p_bandwidth)->set(maxBps * 8);

				} else {
					DBG_DEBUG(FMT("%1%: invalid maximumBps") % getId());

				}

				int freeBps = pt.get("freeBps", maxBps); // bytes per second
				if (freeBps > 0) {
					DBG_DEBUG(FMT("%1%: setting %2% freeBps to %3%") % getId() % naName % freeBps);
					na->getProperty<CIntValue>(INetAdapt::p_freeBandwidth)->set(freeBps * 8);

				} else {
					DBG_DEBUG(FMT("%1%: invalid freeBps") % getId());

				}

//				ptree::iterator it;
//				for (it = pt.begin(); it != pt.end(); it++) {
//					ptree::value_type v = *it;
//					DBG_DEBUG(FMT("%1%: pt: %2% - %3%") % getId() % v.first % v.second.data());
//
//				}

			} else {
				DBG_DEBUG(FMT("%1%: empty netadapt settings message") % getId());

			}

		} else {
			DBG_ERROR(FMT("%1%: %2% not found") % getId() % naName);

		}

	} else if ((uri == "nena://localhost/sillberg/capabilities") && (m == IAppConnector::method_put)) {
		if (mbuf->size() > 0) {
			char* cs = new char[mbuf->size()+1];
			mbuf->getBuffer().read((boctet_t*) cs);
			cs[mbuf->size()] = 0;
			stringstream ss(cs);
			DBG_DEBUG(FMT("%1%: capabilitiesSelected: %2%") % getId() % ss.str());

			ptree pt;
			read_json(ss, pt);
			delete[] cs;
			cs = NULL;

			string s;
			s = pt.get("messageType", string());
			if (s != "capabilitiesSelected") {
				DBG_DEBUG(FMT("%1%: invalid message type %2%") % getId() % s);
				return;
			}

			ptree caps = pt.get_child("capabilitiesSelected");

			ptree::iterator it;
			for (it = caps.begin(); it != caps.end(); it++) {
				ptree::value_type v = *it;
				ptree cap = v.second.get_child("");

				string uri = cap.get("url", string());
				string na = cap.get("path", string());
				int prio = cap.get("priority", 0);

				DBG_DEBUG(FMT("%1%: URI %2% via %3% (priority %4%)") % getId() % uri % na % prio);

//				ptree::iterator it2;
//				for (it2 = cap.begin(); it2 != cap.end(); it2++) {
//					ptree::value_type v2 = *it2;
//					DBG_DEBUG(FMT("%1%: cap: %2% = %3%") % getId() % v2.first % v2.second.data());
//
//				}

				list<shared_ptr<CFlowState> > flowStates;
				d_func()->nodearch->lookupFlowStates(uri, flowStates);
				if (flowStates.empty()) {
					DBG_DEBUG(FMT("%1%: no flow state found for %2%") % getId() % uri);
					continue;
				}

				if (flowStates.size() > 1) {
					DBG_WARNING(FMT("%1%: more than one flow state found for %2%") % getId() % uri);
				}

				INetAdapt* netAdapt = d_func()->naBroker->getNetAdapt(na);
				if (netAdapt == NULL) {
					DBG_WARNING(FMT("%1%: network adaptor %2% not found") % getId() % na);
					continue;
				}

				shared_ptr<CMorphableValue> p(new CPointerValue<INetAdapt>(netAdapt));
				flowStates.front()->setProperty(IMessage::p_netAdapt, p);

			}



		} else {
			DBG_DEBUG(FMT("%1%: empty capabilitiesSelected message") % getId());

		}

	} else {
		throw EUnhandledMessage();

	}
}

void Management::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{
	throw EUnhandledMessage();
}

const Management::Private * Management::d_func() const
{
	return d.get();
}

Management::Private * Management::d_func()
{
	return d.get();
}

void Management::assemble()
{
	d_func()->sys->initSchedulers(d_func()->nodearch);
	setMessageScheduler(d_func()->sys->getMainScheduler());

	d_func()->netletSelector.reset(new CNetletSelector(d_func()->nodearch, d_func()->nodearch->getDefaultScheduler()));
	d_func()->naBroker.reset(new CNetAdaptBroker(d_func()->nodearch, d_func()->nodearch->getDefaultScheduler()));
	d_func()->rep.reset(new CLocalRepository(d_func()->nodearch, d_func()->nodearch->getDefaultScheduler()));

	DBG_DEBUG("NA: Initializing network accesses");
	list<INetAdapt *> netAdapts;
	list<INetAdapt *>::const_iterator na_it;

	d_func()->sys->initNetAdapts(d_func()->nodearch, d_func()->sys->getMainScheduler());
	d_func()->sys->getNetAdapts(netAdapts);
	for (na_it = netAdapts.begin(); na_it != netAdapts.end(); na_it++) {
		d_func()->naBroker->registerNetAdapt(*na_it);
	}

	DBG_DEBUG("NA: Initializing repository");
	d_func()->rep->initialize();

	// link the different entities (quick and dirty)
	// TODO: handle multiple NAs etc
	if (netAdapts.size() == 0) {
		DBG_ERROR("No NAs detected! Please check configuration.");

	} else {
		if (d_func()->rep->multiplexers.size() == 0) {
			DBG_ERROR("No Multiplexers detected! Please check configuration.");

		} else {
			list<INetletMultiplexer *>::const_iterator m_it;
			for (m_it = d_func()->rep->multiplexers.begin(); m_it != d_func()->rep->multiplexers.end(); m_it++) {
				INetletMultiplexer *multiplexer = *m_it;

				// attach NAs to multiplexer
				list<INetAdapt *> & nas = getNetAdaptBroker()->getNetAdapts(multiplexer->getMetaData()->getArchName());
				list<INetAdapt *>::iterator nas_it;
				for (nas_it = nas.begin(); nas_it != nas.end(); nas_it++)
					(*nas_it)->setPrev(multiplexer);

				// TODO: determine some sort of default network access
				multiplexer->setNext(nas.front());

				if (d_func()->rep->netlets.size() == 0) {
					DBG_ERROR("No Netlets detected! Please check configuration.");

				} else {
					list<INetlet *>::iterator n_it;
					for (n_it = d_func()->rep->netlets.begin(); n_it != d_func()->rep->netlets.end(); n_it++) {
						INetlet *netlet = *n_it;
						if (netlet->getMetaData()->getArchName() == multiplexer->getMetaData()->getArchName()) {
							netlet->setNext(multiplexer);
							netlet->setPrev(d_func()->netletSelector.get());

						}

					}

				}

			}

		}

	}

	/// initialize app interface daemon
	d_func()->sys->initAppServers(d_func()->nodearch, d_func()->sys->getMainScheduler());
	d_func()->sys->getAppServers(d_func()->appServers);
}

void Management::run()
{
	/// start system I/O
	d_func()->sys->run();

	/// start app servers
	list<shared_ptr<IAppServer> >::iterator it;

	for (it = d_func()->appServers.begin(); it != d_func()->appServers.end(); it++) {
		DBG_DEBUG(FMT ("Starting AppServer %1%.") % (*it)->getId());
		(*it)->start();
		(*it)->startAppConnectors();
	}
}

void Management::stop()
{
	/// start system I/O
	d_func()->sys->stop();

	/// stop app servers
	list<shared_ptr<IAppServer> >::iterator it;

	for (it = d_func()->appServers.begin(); it != d_func()->appServers.end(); it++) {
		(*it)->stop();
		(*it)->stopAppConnectors();
	}
}

void Management::disassemble()
{
	d_func()->appServers.clear();
	d_func()->rep.reset(NULL);
	d_func()->netletSelector.reset(NULL);
	d_func()->naBroker.reset(NULL);

	setMessageScheduler(NULL); // unregister us

	d_func()->sys->end();
}

ISystemWrapper * Management::getSys() const
{
	return d_func()->sys;
}

CNetAdaptBroker * Management::getNetAdaptBroker() const
{
	return d_func()->naBroker.get();
}

CNetletSelector * Management::getNetletSelector() const
{
	return d_func()->netletSelector.get();
}

INetletRepository * Management::getRepository() const
{
	return d_func()->rep.get();
}

string Management::processCommand(string msg)
{
//	istringstream confin (msg);
//	xml_parse_result result;
//	xml_document xmlmsg;
//	ostringstream of;
//
//	if (!(result = xmlmsg.load(confin)))
//		throw EConfig (result.description());
//
//	string type = xmlmsg.child("message").attribute("type").value();
//
//	if (type == "request")
//	{
//		if (xmlmsg.child("message").child("show"))
//		{
//			d_func ()->handleShow (xmlmsg);
//			xmlmsg.print(of, "", format_raw);
//		}
//		else
//		{
//			xml_document reply;
//			reply.append_child("message");
//			reply.child("message").append_attribute("type").set_value("error");
//			string str = "Request Type " + string(xmlmsg.first_child ().name()) + " unknown.";
//			reply.child("message").append_child("description").append_child(node_pcdata).set_value(str.c_str ());
//			reply.print(of, "", format_raw);
//		}
//	}
//	else if (type == "command")
//	{
//		xml_node base;
//		xml_document reply;
//		reply.append_child("message");
//
//		if (base =/*=*/ xmlmsg.child("message").child("deploy"))
//		{
//			string name = base.child("netlet").attribute("name").value();
//			string confstring;
//			ostringstream ofstr;
//
//			base.print (ofstr, "", format_raw);
//			confstring = ofstr.str();
//
//			/// first, set config
//			setNetletConfig(confstring);
//
//			/// then, load the netlet
//			try
//			{
//				d_func()->rep->loadNetlet(name);
//				reply.child("message").append_attribute("type").set_value("ack");
//			}
//			catch (...)
//			{
//				reply.child("message").append_attribute("type").set_value("error");
//				reply.child("message").append_child("description").append_child(node_pcdata).set_value("Netlet failed to load!");
//			}
//		}
//		else if (base =/*=*/ xmlmsg.child("message").child("updateConfig"))
//		{
//			string updateType = base.attribute("type").value();
//
//			if (updateType == "add")
//			{
//				{
//					unique_lock<shared_mutex> cfgFileLock(d_func()->cfgFileMutex);
//
//					//ostringstream oss;
//					//d_func()->configFile.print(oss, "", format_raw);
//					//DBG_INFO(FMT("Preintegrated added Config: %1%") % oss.str());
//
//					/// integrate new information to config file
//					integrate (base, d_func()->configFile.child("root"));
//
//					//d_func()->configFile.print(oss, "", format_raw);
//					//DBG_INFO(FMT("Integrated added Config: %1%") % oss.str());
//				}
//
//				/// fake...
//				IMessageProcessor * agent = d_func()->rep->getNetletByName("netlet://edu.kit.tm/itm/simpleArch/agentNetlet");
//				shared_ptr<IMessage> ev(new Event_UpdatedConfig(agent, d_func()->rep->multiplexers.front()));
//				d_func()->nodearch->getDefaultScheduler()->sendMessage(ev);
//
//				reply.child("message").append_attribute("type").set_value("ack");
//
//			}
//			else
//			{
//				reply.child("message").append_attribute("type").set_value("error");
//				reply.child("message").append_child("description").append_child(node_pcdata).set_value(("Configuration update Type " + updateType + " unknown.").c_str());
//			}
//		}
//		else if (base =/*=*/ xmlmsg.child("message").child("stop"))
//		{
//			/// return cool message
//			reply.child("message").append_attribute("type").set_value("ack");
//			reply.child("message").append_child("description").append_child(node_pcdata).set_value("Hammertime!");
//
//			/// signal our own process to terminate
//			DBG_DEBUG(FMT("Management: received stop message, passing to main thread."));
//			kill (getpid(), SIGTERM);
//		}
//
//		reply.print(of, "", format_raw);
//	}
//	else
//	{
//		xml_document reply;
//		reply.append_child("message");
//		reply.child("message").append_attribute("type").set_value("error");
//		string str = "Message Type " + type + " unknown.";
//		reply.child("message").append_child("description").append_child(node_pcdata).set_value(str.c_str ());
//		reply.print(of, "", format_raw);
//	}
//
//	return of.str ();
	return string();
}

//void Management::Private::handleShow (xml_document & xmlmsg)
//{
//	string what = xmlmsg.child("message").child("show").attribute("what").value();
//
//	if (what == "archs")
//	{
//		istringstream stats (rep->getStats());
//		xml_parse_result result;
//		xml_document xmlstats;
//
//		if (!(result = xmlstats.load(stats)))
//			throw EConfig (result.description());
//
//		xmlmsg.child("message").child("show").append_copy(xmlstats.child("stats").child("component").child("archList"));
//	}
//	else if (what == "allNetlets")
//	{
//		istringstream stats (rep->getStats());
//		xml_parse_result result;
//		xml_document xmlstats;
//
//		if (!(result = xmlstats.load(stats)))
//			throw EConfig (result.description());
//
//		xmlmsg.child("message").child("show").append_copy(xmlstats.child("stats").child("component").child("allNetletList"));
//	}
//	else if (what == "loadedNetlets")
//	{
//		istringstream stats (rep->getStats());
//		xml_parse_result result;
//		xml_document xmlstats;
//
//		if (!(result = xmlstats.load(stats)))
//			throw EConfig (result.description());
//
//		xmlmsg.child("message").child("show").append_copy(xmlstats.child("stats").child("component").child("loadedNetletList"));
//	}
//	else if (what == "urls")
//	{
//		istringstream stats (nodearch->getMultiplexer("architecture://edu.kit.tm/itm/simpleArch")->getStats());
//		xml_parse_result result;
//		xml_document xmlstats;
//
//		if (!(result = xmlstats.load(stats)))
//			throw EConfig (result.description());
//
//		xmlmsg.child("message").child("show").append_copy(xmlstats.child("stats").find_child_by_attribute("component", "name", "nameAddrMapper").child("urllist"));
//	}
//	else if (what == "fib")
//	{
//		istringstream stats (nodearch->getMultiplexer("architecture://edu.kit.tm/itm/simpleArch")->getStats());
//		xml_parse_result result;
//		xml_document xmlstats;
//
//		if (!(result = xmlstats.load(stats)))
//			throw EConfig (result.description());
//
//		xmlmsg.child("message").child("show").append_copy(xmlstats.child("stats").find_child_by_attribute("component", "name", "fib").child("fiblist"));
//	}
//	else if (what == "name")
//	{
//		xmlmsg.child("message").child("show").append_child(node_pcdata).set_value(nodearch->getNodeName().c_str());
//	}
//	xmlmsg.child("message").attribute("type").set_value("reply");
//}

