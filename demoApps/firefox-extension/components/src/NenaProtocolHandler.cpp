//#include "nspr.h"
//#include "nsAString.h"
//#include "nsIURL.h"
//#include "nspr.h"
#include "NenaProtocolHandler.h"
#include "NenaChannel.h"
#include "nsStringAPI.h"
#include "nsNetCID.h"
//#include "nsNetUtil.h"
#include "nsNetError.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIURL.h"
//#include "nsCRT.h"
//#include "nsIComponentManager.h"
//#include "nsIServiceManager.h"
//#include "nsIInterfaceRequestor.h"
//#include "nsIInterfaceRequestorUtils.h"
//#include "nsIProgressEventSink.h"
//#include "nsNetError.h"

#include "../../../tmnet/net_c.h"

static NS_DEFINE_CID(kSimpleURICID, NS_SIMPLEURI_CID);

////////////////////////////////////////////////////////////////////////////////
// NenaProtocolHandler methods:

NS_IMPL_ISUPPORTS2(NenaProtocolHandler,
                   INenaProtocolHandler,
                   nsIProtocolHandler)

NenaProtocolHandler::NenaProtocolHandler()
{
	tmnet_init();
}

NenaProtocolHandler::~NenaProtocolHandler()
{
}

////////////////////////////////////////////////////////////////////////////////
// nsIProtocolHandler methods:

NS_IMETHODIMP
NenaProtocolHandler::GetScheme(nsACString &result) {
    result.AssignLiteral("nena");
    return NS_OK;
}

NS_IMETHODIMP
NenaProtocolHandler::GetDefaultPort(PRInt32 *result) {
    // no ports for nena protocol
    *result = -1;
    return NS_OK;
}

NS_IMETHODIMP
NenaProtocolHandler::GetProtocolFlags(PRUint32 *result) {
    *result = URI_LOADABLE_BY_ANYONE | URI_NOAUTH;
    return NS_OK;
}

NS_IMETHODIMP
NenaProtocolHandler::NewURI(const nsACString &aSpec,
                      const char *aCharset, // ignore charset info
                      nsIURI *aBaseURI,
                      nsIURI **result) {
    *result = nsnull;
    nsresult rv;

    nsCOMPtr<nsIURI> url = do_CreateInstance(kSimpleURICID, &rv);
    if (NS_FAILED(rv)) return rv;

    PRInt32 colon = aSpec.Find("nena:");
    if (colon == -1) {
        // relative string
        nsCAutoString spec;
        aBaseURI->GetSpec(spec);
        PRInt32 posLastSlash = spec.RFindChar('/');
        if (posLastSlash == -1) {
            NS_ERROR_MALFORMED_URI;
        }
        spec.SetLength(posLastSlash + 1);
        spec.Append(aSpec);

        rv = url->SetSpec(spec);
        if (NS_FAILED(rv)) return rv;
    } else {
        rv = url->SetSpec(aSpec);
        if (NS_FAILED(rv)) return rv;
    }

    //NS_TryToSetImmutable(url);
    url.swap(*result);
    return rv;
}

NS_IMETHODIMP
NenaProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result) {
    NS_ENSURE_ARG_POINTER(uri);
    NenaChannel* channel = new NenaChannel(uri);
    if (!channel)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(channel);

    /*nsresult rv = channel->Init();
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }*/

    *result = channel;
    return NS_OK;
}

NS_IMETHODIMP
NenaProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval) {
    // don't override anything.
    *_retval = PR_FALSE;
    return NS_OK;
}
