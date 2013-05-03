/*
 * netletTemplate.h
 *
 *  Created on: 12.06.2012
 *      Author: benjamin
 */

#ifndef NETLETTEMPLATE_H_
#define NETLETTEMPLATE_H_

#include <composableNetlet.h>

#include <string>

#include <netlet.h>
#include <exceptions.h>
#include <nena.h>

namespace edu_kit_tm {
namespace itm {
namespace generic {

/**
 * @brief  Netlet meta data template
 *
 * Generic netlet meta data, filled by information from the config file.
 *
 */
class CNetletMetaDataTemplate : public INetletMetaData {
private:
	std::string netletId;
	std::string archName;
	std::string handlerRegEx;
	bool fControlNetlet;
	CNena * nena;

public:
	NENA_EXCEPTION(EMissingConfig);

	CNetletMetaDataTemplate(std::string netletId, CNena *n);
	virtual ~CNetletMetaDataTemplate();

	virtual std::string getArchName() const;
	virtual const std::string& getId() const;

	virtual bool isControlNetlet() const;
	virtual int canHandle(const std::string & uri, std::string & req) const;

	virtual INetlet* createNetlet(CNena *nena, IMessageScheduler *sched); // factory function
};

class CNetletTemplate : public IComposableNetlet {
private:
	CNetletMetaDataTemplate *netletMetaData;

	boost::shared_ptr<IBuildingBlock> upmostBB, downmostBB;
public:
	/**
	 * @param
	 */
	CNetletTemplate(CNena *nena, IMessageScheduler *sched, std::string id, CNetletMetaDataTemplate *nmd);
	virtual ~CNetletTemplate();

	/// from IMessageProcessor
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);

	/// from INetlet
	virtual INetletMetaData * getMetaData() const;
};

}
}
}


#endif /* NETLETTEMPLATE_H_ */
