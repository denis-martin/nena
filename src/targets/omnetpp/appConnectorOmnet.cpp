/*
 * appConnectorOmnet.cpp
 *
 *  Created on: Jul 21, 2009
 *      Author: denis
 */

#include "appConnectorOmnet.h"

CAppConnectorOmnet::CAppConnectorOmnet(IMessageScheduler* sched):
	IAppConnector(sched)
{
	className += "::CAppConnectorOmnet";
}

CAppConnectorOmnet::~CAppConnectorOmnet()
{
}
