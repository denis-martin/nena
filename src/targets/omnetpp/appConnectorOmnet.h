/*
 * appConnectorOmnet.h
 *
 *  Created on: Jul 21, 2009
 *      Author: denis
 */

#ifndef APPCONNECTOROMNET_H_
#define APPCONNECTOROMNET_H_

#include "appConnector.h"

/**
 * @brief	Omnet implementation of the app connector interface. This
 * 			actually does nothing since the main logic will be in the
 * 			traffic generator applications themselves.
 */
class CAppConnectorOmnet : public IAppConnector
{
public:
	CAppConnectorOmnet(IMessageScheduler* sched);
	virtual ~CAppConnectorOmnet();
};

#endif /* APPCONNECTOROMNET_H_ */
