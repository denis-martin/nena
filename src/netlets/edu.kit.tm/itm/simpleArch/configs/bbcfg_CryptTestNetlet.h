/** @file
 * bbcfg_CryptTestNetlet.h
 *
 * Created on:
 * Generator: NetletEdit
 */

#ifndef BBCFG_CRYPTTESTNETLET_H_
#define BBCFG_CRYPTTESTNETLET_H_

#include "composableNetlet.h"

//#include "edu.kit.tm/itm/crypt/benchmark/bb_bench.h"
#include "edu.kit.tm/itm/crypt/bb_frag.h"
#include "edu.kit.tm/itm/crypt/bb_header.h"
#include "edu.kit.tm/itm/crypt/bb_pad.h"
#include "edu.kit.tm/itm/crypt/bb_enc.h"
#include "edu.kit.tm/itm/crypt/bb_crc.h"

/**
 * @brief       Name space used by the NetletEdit tool
 */
namespace netletEdit {

/**
 * @brief       NOT Auto-generated Netlet configuration
 */
class CryptTestNetletConfig : public IComposableNetlet::Config
{
public:
	CryptTestNetletConfig()
	{
		// TODO instead of instantiating the BBs directly, a factory should be used
		//outgoingChain.push_back("bb://edu.kit.tm/itm/crypt/benchmark/Bb_Bench");
		outgoingChain.push_back("bb://edu.kit.tm/itm/crypt/bb_Frag");
		outgoingChain.push_back("bb://edu.kit.tm/itm/crypt/Bb_Pad");
		outgoingChain.push_back("bb://edu.kit.tm/itm/crypt/Bb_Enc");
		outgoingChain.push_back("bb://edu.kit.tm/itm/crypt/Bb_Header");
		outgoingChain.push_back("bb://edu.kit.tm/itm/crypt/Bb_CRC");

		// same for incoming
		std::list<std::string>::iterator it;
		for (it = outgoingChain.begin(); it != outgoingChain.end(); it++) {
			incomingChain.push_front(*it);
		}
	};

	virtual ~CryptTestNetletConfig()
	{
		outgoingChain.clear();
		incomingChain.clear();
	};
};


} // namespace netletEdit

#endif /* BBCFG_CRYPTTESTNETLET_H_ */
