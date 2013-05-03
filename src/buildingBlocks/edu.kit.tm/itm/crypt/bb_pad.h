/** @file
 * bb_pad.h
 *
 * @brief Crypt Test Padding Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _BB_PAD_H_
#define _BB_PAD_H_

#include "composableNetlet.h"

#include "nena.h"

namespace edu_kit_tm {
namespace itm {
namespace crypt {

extern const std::string padClassName;

class Bb_Pad : public IBuildingBlock
{
	public:
	Bb_Pad(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_Pad();

	// from IMessageProcessor
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
};

} // maam
} // itm
} // edu.kit.tm

#endif /* _BB_PAD_H_ */
