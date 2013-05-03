#include <composableNetlet.h>

#include <string>
#include <set>
#include <boost/shared_ptr.hpp>

#include <composableNetlet.h>

#include "arq/bb_arqStopAndWait.h"
#include "arq/bb_arqGoBackN.h"
#include "segment/bb_simpleSegment.h"

using namespace std;
using namespace edu_kit_tm::itm::transport;
using boost::shared_ptr;

/**
 * @brief return the avaiable ids
 *
 * @return list of strings with the class ids of the building blocks in this shared library
 */
extern "C" set<string> getBuildingBlockClasses()
{
	set<string> ids;
	
	ids.insert(arqStopAndWaitClassName);
	ids.insert(BB_ARQGOBACKN_ID);
	ids.insert(BB_SIMPLESEGMENT_ID);

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

	return ptr;
}
