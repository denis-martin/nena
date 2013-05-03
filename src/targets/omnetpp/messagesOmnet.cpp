
#include "messagesOmnet.h"
#include "systemOmnet.h"
#include "debug.h"

#include <string>

using namespace std;
using boost::shared_ptr;

static const std::string & schedulerName = "scheduler://omnet/simple";

/* ************************************************************************* */

// The module class needs to be registered with OMNeT++
Define_Module(COmnetScheduler);

COmnetScheduler::COmnetScheduler(IMessageScheduler *parent):
	CSimpleScheduler(parent)
{
	name += "::COmnetScheduler";
}

COmnetScheduler::~COmnetScheduler()
{

}

/**
 * cSimpleModule::initialize()
 */
void COmnetScheduler::initialize()
{
}

/**
 * cSimpleModule::handleMessage()
 */
void COmnetScheduler::handleMessage(cMessage *msg)
{
	DBG_FAIL("COmnetScheduler::handleMessage() not yet implemented");
}

/**
 * @brief Set a single shot time
 *
 * @param timer	Timer to be set
 */
void COmnetScheduler::setTimer(boost::shared_ptr<CTimer>& timer)
{
	cMessage *omsg = new cMessage(("timeout::" + timer->getClassName()).c_str());
	scheduleAt(simTime() + timer->timeout, omsg);

	events[omsg] = timer;
	reverseEvents[timer] = omsg;
};

void COmnetScheduler::cancelTimer(boost::shared_ptr<CTimer>& timer)
{
	map<shared_ptr<IMessage>, cMessage *>::iterator rit = reverseEvents.find(timer);
	if (rit != reverseEvents.end()) {
		EventMap::iterator it = events.find(rit->second);
		if (it != events.end()) {
			events.erase(it);
		}

		cancelEvent(rit->second);
		reverseEvents.erase(rit);
	}
}

/**
 * @brief Unregister a message processor belonging to this scheduler
 */
void COmnetScheduler::unregisterMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor)
{
	CSimpleScheduler::unregisterMessageProcessor(proc);

	// Cancel OMNeT messages belonging to this processor
	map<cMessage *, shared_ptr<IMessage> >::iterator it;
	for (it = events.begin(); it != events.end(); /* nothing */) {
		shared_ptr<IMessage> msg = it->second;

		map<shared_ptr<IMessage>, cMessage *>::iterator rit = reverseEvents.find(msg);
		if (rit != reverseEvents.end()) {
			reverseEvents.erase(rit);
		}

		if (proc == msg->getTo()) {
			cancelEvent(it->first);

			map<cMessage *, shared_ptr<IMessage> >::iterator tmpit = it;
			it++;

			events.erase(tmpit);

		} else {
			it++;

		}
	}
};

void COmnetScheduler::passMessage (boost::shared_ptr<IMessage> msg)
{
	throw ENotResponsible ("This Scheduler does not support message passing!");
}

const std::string & COmnetScheduler::getId () const
{
	return schedulerName;
}
