/** @file
 * bb_frag.h
 *
 * @brief Fragmentation Building Block
 *
 *  Created on: Apr 18, 2010
 *      Author: Benjamin Behringer
 */

#ifndef _BB_FRAG_H_
#define _BB_FRAG_H_

#include "composableNetlet.h"

#include "nena.h"

namespace edu_kit_tm {
namespace itm {
namespace crypt {

extern const std::string fragClassName;

class Bb_Frag : public IBuildingBlock
{
	public:
	Bb_Frag(CNena *nodeArch, IMessageScheduler *sched, IComposableNetlet *netlet, const std::string id);
	virtual ~Bb_Frag();

	// from IMessageProcessor
	virtual void processEvent(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processTimer(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processOutgoing(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
	virtual void processIncoming(boost::shared_ptr<IMessage> msg) throw (EUnhandledMessage);
};

} // maam
} // itm
} // edu.kit.tm

#endif /* _BB_FRAG_H_ */
