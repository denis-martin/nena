/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

//#include "nsIOService.h"
#include "NenaChannel.h"
#include "NenaProtocolHandler.h"
#include "NenaInputStream.h"
//#include "nsNetUtil.h"
#include "nsIURI.h"
#include "nsITransport.h"
#include "nsMimeTypes.h"
#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsNetError.h"
#include "nsCOMArray.h"
#include "nsIContentSniffer.h"
#include "nsNetCID.h"
#include "nsComponentManagerUtils.h"
//#include "nsReadableUtils.h"
//#include "nsEscape.h"
//#include "plbase64.h"
//#include "plstr.h"
//#include "prmem.h"

//-----------------------------------------------------------------------------
// Helper functions/classes

static NS_DEFINE_CID(kInputStreamPumpCID, NS_INPUTSTREAMPUMP_CID);

// This class is used to suspend a request across a function scope.
class ScopedRequestSuspender {
public:
  ScopedRequestSuspender(nsIRequest *request)
    : mRequest(request) {
    if (mRequest && NS_FAILED(mRequest->Suspend())) {
      NS_WARNING("Couldn't suspend pump");
      mRequest = nsnull;
    }
  }
  ~ScopedRequestSuspender() {
    if (mRequest)
      mRequest->Resume();
  }
private:
  nsIRequest *mRequest;
};

// Used to suspend data events from mPump within a function scope.  This is
// usually needed when a function makes callbacks that could process events.
#define SUSPEND_PUMP_FOR_SCOPE() \
  ScopedRequestSuspender pump_suspender__(mPump)


//-----------------------------------------------------------------------------
// NenaChannel

NenaChannel::NenaChannel()
  : mLoadFlags(LOAD_NORMAL)
  , mQueriedProgressSink(PR_TRUE)
  , mSynthProgressEvents(PR_FALSE)
  , mWasOpened(PR_FALSE)
  , mWaitingOnAsyncRedirect(PR_FALSE)
  , mStatus(NS_OK)
{
  //mContentType.Assign(NS_LITERAL_CSTRING(UNKNOWN_CONTENT_TYPE));
  mContentType.Assign(NS_LITERAL_CSTRING("text/plain"));
  mContentCharset.Assign(NS_LITERAL_CSTRING("UTF-8"));
  mContentLength = -1;
}

NenaChannel::~NenaChannel()
{
}

nsresult
NenaChannel::BeginPumpingData()
{
  nsCOMPtr<nsIInputStream> stream ;
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = OpenContentStream(PR_TRUE, getter_AddRefs(stream),
                                  getter_AddRefs(channel));
  if (NS_FAILED(rv))
    return rv;

  NS_ASSERTION(!stream || !channel, "Got both a channel and a stream?");

  // this feature is from nsBaseChannel
  // I keep the same API, so if nsBaseChannel will be exported some day,
  // there is little work to do
  /*if (channel) {
      rv = NS_DispatchToCurrentThread(new RedirectRunnable(this, channel));
      if (NS_SUCCEEDED(rv))
          mWaitingOnAsyncRedirect = PR_TRUE;
      return rv;
  }*/

  // By assigning mPump, we flag this channel as pending (see IsPending).  It's
  // important that the pending flag is set when we call into the stream (the
  // call to AsyncRead results in the stream's AsyncWait method being called)
  // and especially when we call into the loadgroup.  Our caller takes care to
  // release mPump if we return an error.

  mPump = do_CreateInstance(kInputStreamPumpCID, &rv);
  if (NS_FAILED(rv)) return rv;
  rv = mPump->Init(stream, -1, -1, 0, 0, PR_TRUE);
  //rv = nsInputStreamPump::Create(getter_AddRefs(mPump), stream, -1, -1, 0, 0,
  //                               PR_TRUE);
  if (NS_SUCCEEDED(rv))
    rv = mPump->AsyncRead(this, nsnull);

  return rv;
}

nsresult
NenaChannel::OpenContentStream(PRBool async, nsIInputStream **result,
                                 nsIChannel** channel)
{
    NS_ENSURE_TRUE(URI(), NS_ERROR_NOT_INITIALIZED);

    nsresult rv;

    nsCAutoString spec;
    rv = URI()->GetAsciiSpec(spec);
    if (NS_FAILED(rv)) return rv;
    nsDependentCSubstring_external uri = Substring(spec, 5); // remove nena:

    NenaInputStream* inputStream = new NenaInputStream(uri);
    if (!inputStream)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(inputStream);
    nsCString metadata;
    inputStream->getMetadata(metadata);
    SetContentType(metadata);


    //SetContentCharset(mContentCharset);
    //SetContentLength(44);

    *result = inputStream;

    return NS_OK;
}

//-----------------------------------------------------------------------------
// NenaChannel::nsISupports

NS_IMPL_ISUPPORTS3(NenaChannel,
                             //nsHashPropertyBag,
                             nsIRequest,
                             nsIChannel,
                             //nsIInterfaceRequestor,
                             //nsITransportEventSink,
                             //nsIRequestObserver,
                             nsIStreamListener)
                             //nsIAsyncVerifyRedirectCallback)

//-----------------------------------------------------------------------------
// NenaChannel::nsIRequest

NS_IMETHODIMP
NenaChannel::GetName(nsACString &result)
{
  if (!mURI) {
    result.Truncate();
    return NS_OK;
  }
  return mURI->GetSpec(result);
}

NS_IMETHODIMP
NenaChannel::IsPending(PRBool *result)
{
  *result = IsPending();
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetStatus(nsresult *status)
{
  if (mPump && NS_SUCCEEDED(mStatus)) {
    mPump->GetStatus(status);
  } else {
    *status = mStatus;
  }
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::Cancel(nsresult status)
{
  // Ignore redundant cancelation
  if (NS_FAILED(mStatus))
    return NS_OK;

  mStatus = status;

  if (mPump)
    mPump->Cancel(status);

  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::Suspend()
{
  NS_ENSURE_TRUE(mPump, NS_ERROR_NOT_INITIALIZED);
  return mPump->Suspend();
}

NS_IMETHODIMP
NenaChannel::Resume()
{
  NS_ENSURE_TRUE(mPump, NS_ERROR_NOT_INITIALIZED);
  return mPump->Resume();
}

NS_IMETHODIMP
NenaChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
  NS_IF_ADDREF(*aLoadGroup = mLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
  mLoadGroup = aLoadGroup;
  //CallbacksChanged();
  return NS_OK;
}

//-----------------------------------------------------------------------------
// NenaChannel::nsIChannel

NS_IMETHODIMP
NenaChannel::GetOriginalURI(nsIURI **aURI)
{
  *aURI = mOriginalURI;
  NS_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetOriginalURI(nsIURI *aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  mOriginalURI = aURI;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetURI(nsIURI **aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetOwner(nsISupports **aOwner)
{
  NS_IF_ADDREF(*aOwner = mOwner);
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetOwner(nsISupports *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks)
{
  NS_IF_ADDREF(*aCallbacks = mCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks)
{
  mCallbacks = aCallbacks;
  //CallbacksChanged();
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetContentType(nsACString &aContentType)
{
  aContentType = mContentType;
  //aContentType.AssignLiteral("text/html");
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetContentType(const nsACString &aContentType)
{
  // mContentCharset is unchanged if not parsed
  //PRBool dummy;
  //net_ParseContentType(aContentType, mContentType, mContentCharset, &dummy);
  mContentType = aContentType;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetContentCharset(nsACString &aContentCharset)
{
  aContentCharset.Assign(mContentCharset);
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetContentCharset(const nsACString &aContentCharset)
{
  mContentCharset = aContentCharset;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::GetContentLength(PRInt32 *aContentLength)
{
  *aContentLength = mContentLength;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::SetContentLength(PRInt32 aContentLength)
{
  mContentLength = aContentLength;
  return NS_OK;
}

NS_IMETHODIMP
NenaChannel::Open(nsIInputStream **result)
{
  // we are not required to implement this...
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
NenaChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctxt)
{
  NS_ENSURE_TRUE(mURI, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!mPump, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);
  NS_ENSURE_ARG(listener);

  // Store the listener and context early so that OpenContentStream and the
  // stream's AsyncWait method (called by AsyncRead) can have access to them
  // via PushStreamConverter and the StreamListener methods.  However, since
  // this typically introduces a reference cycle between this and the listener,
  // we need to be sure to break the reference if this method does not succeed.
  mListener = listener;
  mListenerContext = ctxt;

  // This method assigns mPump as a side-effect.  We need to clear mPump if
  // this method fails.
  nsresult rv = BeginPumpingData();
  if (NS_FAILED(rv)) {
    mPump = nsnull;
    mListener = nsnull;
    mListenerContext = nsnull;
    mCallbacks = nsnull;
    return rv;
  }

  // At this point, we are going to return success no matter what.

  mWasOpened = PR_TRUE;

  SUSPEND_PUMP_FOR_SCOPE();

  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nsnull);

  //ClassifyURI();

  return NS_OK;
}

//-----------------------------------------------------------------------------
// NenaChannel::nsIRequestObserver

static void
CallTypeSniffers(void *aClosure, const PRUint8 *aData, PRUint32 aCount)
{
  /*nsIChannel *chan = static_cast<nsIChannel*>(aClosure);

  const nsCOMArray<nsIContentSniffer>& sniffers =
    gIOService->GetContentSniffers();
  PRUint32 length = sniffers.Count();
  for (PRUint32 i = 0; i < length; ++i) {
    nsCAutoString newType;
    nsresult rv =
      sniffers[i]->GetMIMETypeFromContent(chan, aData, aCount, newType);
    if (NS_SUCCEEDED(rv) && !newType.IsEmpty()) {
      chan->SetContentType(newType);
      break;
    }
  }*/
}

static void
CallUnknownTypeSniffer(void *aClosure, const PRUint8 *aData, PRUint32 aCount)
{
  /*nsIChannel *chan = static_cast<nsIChannel*>(aClosure);

  nsCOMPtr<nsIContentSniffer> sniffer =
    do_CreateInstance(NS_GENERIC_CONTENT_SNIFFER);
  if (!sniffer)
    return;

  nsCAutoString detected;
  nsresult rv = sniffer->GetMIMETypeFromContent(chan, aData, aCount, detected);
  if (NS_SUCCEEDED(rv))
    chan->SetContentType(detected);*/
}

NS_IMETHODIMP
NenaChannel::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  /*// If our content type is unknown, then use the content type sniffer.  If the
  // sniffer is not available for some reason, then we just keep going as-is.
  if (NS_SUCCEEDED(mStatus) && mContentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE)) {
    mPump->PeekStream(CallUnknownTypeSniffer, static_cast<nsIChannel*>(this));
  }

  // Now, the general type sniffers. Skip this if we have none.
  if ((mLoadFlags & LOAD_CALL_CONTENT_SNIFFERS) &&
      gIOService->GetContentSniffers().Count() != 0)
    mPump->PeekStream(CallTypeSniffers, static_cast<nsIChannel*>(this));*/

  SUSPEND_PUMP_FOR_SCOPE();

  return mListener->OnStartRequest(this, mListenerContext);
}

NS_IMETHODIMP
NenaChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                             nsresult status)
{
  // If both mStatus and status are failure codes, we keep mStatus as-is since
  // that is consistent with our GetStatus and Cancel methods.
  if (NS_SUCCEEDED(mStatus))
    mStatus = status;

  // Cause IsPending to return false.
  mPump = nsnull;

  mListener->OnStopRequest(this, mListenerContext, mStatus);
  mListener = nsnull;
  mListenerContext = nsnull;

  // No need to suspend pump in this scope since we will not be receiving
  // any more events from it.

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, mStatus);

  // Drop notification callbacks to prevent cycles.
  mCallbacks = nsnull;
  //CallbacksChanged();

  return NS_OK;
}

/* readonly attribute unsigned long contentDisposition; */
NS_IMETHODIMP
NenaChannel::GetContentDisposition(PRUint32 *aContentDisposition)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute AString contentDispositionFilename; */
NS_IMETHODIMP
NenaChannel::GetContentDispositionFilename(nsAString & aContentDispositionFilename)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute ACString contentDispositionHeader; */
NS_IMETHODIMP
NenaChannel::GetContentDispositionHeader(nsACString & aContentDispositionHeader)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// NenaChannel::nsIStreamListener

NS_IMETHODIMP
NenaChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                               nsIInputStream *stream, PRUint32 offset,
                               PRUint32 count)
{
  SUSPEND_PUMP_FOR_SCOPE();

  nsresult rv = mListener->OnDataAvailable(this, mListenerContext, stream,
                                           offset, count);
  /*if (mSynthProgressEvents && NS_SUCCEEDED(rv)) {
    PRUint64 prog = PRUint64(offset) + count;
    PRUint64 progMax = ContentLength64();
    OnTransportStatus(nsnull, nsITransport::STATUS_READING, prog, progMax);
  }*/

  return rv;
}

