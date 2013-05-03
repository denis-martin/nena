#ifndef _NENA_PROTOCOL_HANDLER_H_
#define _NENA_PROTOCOL_HANDLER_H_

#include "INenaProtocolHandler.h"

// with this contract-id, the "nena:" uri-schema will be registered in firefox
#define NENA_PROTOCOL_HANDLER_CONTRACTID "@mozilla.org/network/protocol;1?name=nena"
#define NENA_PROTOCOL_HANDLER_CLASSNAME "protocol handler for nena-protocols"
#define NENA_PROTOCOL_HANDLER_CID {0x39c296e8, 0x47b4, 0xa26e, {0x86, 0xd4, 0x3a, 0x99, 0x25, 0x95}}


class NenaProtocolHandler : public INenaProtocolHandler
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_INENAPROTOCOLHANDLER

    NenaProtocolHandler();

private:
    ~NenaProtocolHandler();

protected:
    /* additional members */
};

#endif // of _NENA_PROTOCOL_HANDLER_H_
