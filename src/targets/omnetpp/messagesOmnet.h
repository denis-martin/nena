

#ifndef OMNETSCHEDULER_H_
#define OMNETSCHEDULER_H_

#include "messages.h"
#include "simpleScheduler.h"

#include <map>
#include <boost/shared_ptr.hpp>

#include <omnetpp.h>

typedef std::map<cMessage *, boost::shared_ptr<IMessage> > EventMap;

/**
 * @brief	OMNeT implementation of a message scheduler
 */
class COmnetScheduler : public CSimpleScheduler, public cSimpleModule
{
protected:
	EventMap events;							///< List of pending events
	std::map<boost::shared_ptr<IMessage>, cMessage *> reverseEvents;	///< reverse event lookup

	virtual void initialize();					///< From cSimpleModule
	virtual void handleMessage(cMessage *msg);	///< Handle an OMNeT message

public:
	COmnetScheduler(IMessageScheduler *parent = NULL);
	virtual ~COmnetScheduler();
	/**
	 * @brief Set a single shot time
	 *
	 * @param timer	Timer to be set
	 */
	virtual void setTimer(boost::shared_ptr<CTimer>& timer);

	/**
	 * @brief Cancel a timer
	 *
	 * @param timer Timer to cancel
	 */
	virtual void cancelTimer(boost::shared_ptr<CTimer>& timer);

	/**
	 * @brief Unregister a message processor belonging to this scheduler
	 */
	virtual void unregisterMessageProcessor(IMessageProcessor *proc) throw (EUnknowMessageProcessor);

	virtual bool hasMessageProcessor (IMessageProcessor * proc)
	{
		return false;
	}

	virtual void passMessage (boost::shared_ptr<IMessage> msg);

	virtual const std::string & getId () const;

};

#endif // OMNETSCHEDULER_H_
