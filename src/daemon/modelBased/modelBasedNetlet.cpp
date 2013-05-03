/*
 * modelBasedNetlet.cpp
 *
 *  Created on: 10 Aug 2011
 *      Author: denis
 */

#include "modelBasedNetlet.h"

#include "bb_lua.h"

using namespace std;

CModelBasedNetlet::CModelBasedNetlet(CNena *nena, IMessageScheduler *sched) :
	IComposableNetlet(nena, sched)
{
	IBuildingBlock* bb;

	bb = new Bb_Lua(nena, sched, this);

	bbs.push_back(bb);
}

CModelBasedNetlet::~CModelBasedNetlet()
{
	list<IBuildingBlock*>::iterator it;
	for (it = bbs.begin(); it != bbs.end(); it++) {
		delete *it;
	}
	bbs.clear();
}

/**
 * @brief Process an event message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CModelBasedNetlet::processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{

}

/**
 * @brief Process a timer message directed to this message processing unit
 *
 * @param msg	Pointer to message
 */
void CModelBasedNetlet::processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{

}

/**
 * @brief Process an outgoing message directed towards the network.
 *
 * @param msg	Pointer to message
 */
void CModelBasedNetlet::processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{

}

/**
 * @brief Process an incoming message directed towards the application.
 *
 * @param msg	Pointer to message
 */
void CModelBasedNetlet::processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage)
{

}

/**
 * @brief	Returns the Netlet's meta data
 */
INetletMetaData* CModelBasedNetlet::getMetaData()
{
	return NULL;
}
