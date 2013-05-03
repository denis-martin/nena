#include <composableNetlet.h>

#include <vector>
#include <set>
#include <boost/shared_ptr.hpp>

#include <composableNetlet.h>

#include "bb_pad.h"
#include "bb_header.h"
#include "bb_frag.h"
#include "bb_enc.h"
#include "bb_crc.h"

using namespace std;
using namespace edu_kit_tm::itm::crypt;
using boost::shared_ptr;

/**
 * @brief return the avaiable ids
 *
 * @return set of strings with the class ids of the building blocks in this shared library
 */
extern "C" set<string> getBuildingBlockClasses()
{
	set<string> ids;
	
	ids.insert(padClassName);
	ids.insert(headerClassName);
	ids.insert(fragClassName);
	ids.insert(encClassName);
	ids.insert(crcClassName);

	return ids;
}

/**
 * @brief return an instance of the building block
 *
 * If only the class of the building block is specified, a random id
 * is added. Otherwise the given id is used.
 *
 * @param nena		pointer to nena interface
 * @param sched		scheduler to use for bb
 * @param netlet	netlet using the building block
 * @param classid	class identifier of the building block
 * @param id		id of the building block
 *
 * @return 		pointer to instance of the building block
 */
extern "C" shared_ptr<IBuildingBlock> instantiateBuildingBlock(CNena *nena, IMessageScheduler *sched, IComposableNetlet *netlet, string classid, string id)
{
	shared_ptr<IBuildingBlock> ptr;
	string bbId = classid + "/" + id;


	if (classid == padClassName) {
		ptr.reset(new Bb_Pad(nena, sched, netlet, bbId));
	} else if(classid == headerClassName) {
		ptr.reset(new Bb_Header(nena, sched, netlet, bbId));
	} else if(classid == fragClassName) {
		ptr.reset(new Bb_Frag(nena, sched, netlet, bbId));
	} else if(classid == encClassName) {
		ptr.reset(new Bb_Enc(nena, sched, netlet, bbId));
	} else if(classid == crcClassName) {
		ptr.reset(new Bb_CRC(nena, sched, netlet, bbId));
	}

	return ptr;
}
