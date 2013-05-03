#include <composableNetlet.h>

#include <string>
#include <set>
#include <boost/shared_ptr.hpp>

#include <composableNetlet.h>

#include "video/vid_serializer.h"
#include "video/vid_quantizer.h"
#include "video/vid_idct.h"
#include "video/vid_converter.h"
//#include "video/vid_codec.h"

using namespace std;
using boost::shared_ptr;

/**
 * @brief return the avaiable ids
 *
 * @return set of strings with the class ids of the building blocks in this shared library
 */
extern "C" set<string> getBuildingBlockClasses()
{
	set<string> ids;
	
	ids.insert(ividSerializerClassName);
	ids.insert(ividSerializerNofecClassName);
	ids.insert(ividSerializerLofecClassName);
	ids.insert(ividSerializerHifecClassName);

	ids.insert(ividQuantizerClassName);
	ids.insert(ividQuantizerNofecClassName);
	ids.insert(ividQuantizerLofecClassName);
	ids.insert(ividQuantizerHifecClassName);

	ids.insert(ividIdctClassName);

	ids.insert(ividConverterClassName);

//	ids.insert(ividCodecClassName);

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
