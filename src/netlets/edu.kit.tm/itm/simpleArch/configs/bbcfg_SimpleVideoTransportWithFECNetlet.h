/** @file
 * bbcfg_SimpleVideoTransportWithFECNetlet.h
 * 
 * DO NOT EDIT - auto-generated from
 * bbcfg_SimpleVideoTransportWithFECNetlet.xml
 *
 * Created on: 
 * Generator: NetletEdit
 */

#ifndef BBCFG_SIMPLEVIDEOTRANSPORTWITHFECNETLET_H_
#define BBCFG_SIMPLEVIDEOTRANSPORTWITHFECNETLET_H_

#include "composableNetlet.h"

#include "edu.kit.tm/itm/demo/video/vid_converter.h"
#include "edu.kit.tm/itm/demo/video/vid_idct.h"
#include "edu.kit.tm/itm/demo/video/vid_quantizer.h"
#include "edu.kit.tm/itm/demo/video/vid_serializer.h"
#include "edu.kit.tm/itm/transport/traffic/bb_simpleSmooth.h"
#include "edu.kit.tm/itm/transport/traffic/bb_simpleMultiStreamer.h"

/**
 * @brief       Name space used by the NetletEdit tool
 */
namespace netletEdit {

/**
 * @brief       Auto-generated Netlet configuration
 */
class SimpleVideoTransportWithFECNetletConfig : public IComposableNetlet::Config
{
public:
	SimpleVideoTransportWithFECNetletConfig()
	{
		// TODO instead of instantiating the BBs directly, a factory should be used
		outgoingChain.push_back("bb://edu.kit.tm/itm/demo/video/BB_Vid_Converter");
		outgoingChain.push_back("bb://edu.kit.tm/itm/demo/video/BB_Vid_IDCT");
		outgoingChain.push_back("bb://edu.kit.tm/itm/demo/video/BB_Vid_Quantizer_HiFEC");
		outgoingChain.push_back("bb://edu.kit.tm/itm/demo/video/BB_Vid_Serializer_HiFEC");
		outgoingChain.push_back("bb://edu.kit.tm/itm/transport/traffic/simpleSmooth");
		outgoingChain.push_back("bb://edu.kit.tm/itm/transport/traffic/simpleMultiStreamer");
		
		// same for incoming
		std::list<std::string>::iterator it;
		for (it = outgoingChain.begin(); it != outgoingChain.end(); it++) {
			incomingChain.push_front(*it);
		}
	};
	
	virtual ~SimpleVideoTransportWithFECNetletConfig()
	{
		outgoingChain.clear();
		incomingChain.clear();
	};
};


} // namespace netletEdit

#endif /* BBCFG_SIMPLEVIDEOTRANSPORTWITHFECNETLET_H_ */

