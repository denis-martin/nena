


// the content of this file changed with gecko 2.0
// example: http://mxr.mozilla.org/mozilla-central/source/xpcom/sample/nsSampleModule.cpp

#include "mozilla/ModuleUtils.h"
#include "NenaProtocolHandler.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(NenaProtocolHandler)

// The following line defines a kNS_SAMPLE_CID CID variable.
NS_DEFINE_NAMED_CID(NENA_PROTOCOL_HANDLER_CID);

static const mozilla::Module::CIDEntry kNenaProtocolsCIDs[] = {
    { &kNENA_PROTOCOL_HANDLER_CID, false, NULL, NenaProtocolHandlerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kNenaProtocolsContracts[] = {
    { NENA_PROTOCOL_HANDLER_CONTRACTID, &kNENA_PROTOCOL_HANDLER_CID },
    { NULL }
};

static const mozilla::Module kNenaProtocolsModule = {
    mozilla::Module::kVersion,
    kNenaProtocolsCIDs,
    kNenaProtocolsContracts,
    NULL
};

NSMODULE_DEFN(nsSampleModule) = &kNenaProtocolsModule;

// backward-compatible 
NS_IMPL_MOZILLA192_NSGETMODULE(&kNenaProtocolsModule)
