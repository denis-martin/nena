#ifndef NenaChannel_h___
#define NenaChannel_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsStringAPI.h"
#include "nsIChannel.h"
#include "nsIStreamListener.h"
#include "nsInputStreamPump.h"

class NenaChannel : public nsIChannel
                  , private nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHANNEL
  NS_DECL_NSIREQUEST

  NenaChannel();
  NenaChannel(nsIURI *uri) {
      SetURI(uri);
  }


private:
  ~NenaChannel();

  // Implemented by subclass to supply data stream.  The parameter, async, is
  // true when called from nsIChannel::AsyncOpen and false otherwise.  When
  // async is true, the resulting stream will be used with a nsIInputStreamPump
  // instance.  This means that if it is a non-blocking stream that supports
  // nsIAsyncInputStream that it will be read entirely on the main application
  // thread, and its AsyncWait method will be called whenever ReadSegments
  // returns NS_BASE_STREAM_WOULD_BLOCK.  Otherwise, if the stream is blocking,
  // then it will be read on one of the background I/O threads, and it does not
  // need to implement ReadSegments.  If async is false, this method may return
  // NS_ERROR_NOT_IMPLEMENTED to cause the basechannel to implement Open in
  // terms of AsyncOpen (see NS_ImplementChannelOpen).
  // A callee is allowed to return an nsIChannel instead of an nsIInputStream.
  // That case will be treated as a redirect to the new channel.  By default
  // *channel will be set to null by the caller, so callees who don't want to
  // return one an just not touch it.
  nsresult OpenContentStream(PRBool async, nsIInputStream **stream,
                                     nsIChannel** channel);

protected:
  /* additional members */

public:
  // This is a short-cut to calling nsIRequest::IsPending()
  PRBool IsPending() const {
    return mPump || mWaitingOnAsyncRedirect;
  }

private:
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  // Called to setup mPump and call AsyncRead on it.
  nsresult BeginPumpingData();

  //nsRefPtr<nsInputStreamPump>         mPump;
  nsCOMPtr<nsIInputStreamPump>        mPump;
  nsCOMPtr<nsIInterfaceRequestor>     mCallbacks;
  nsCOMPtr<nsIProgressEventSink>      mProgressSink;
  nsCOMPtr<nsIURI>                    mOriginalURI;
  nsCOMPtr<nsIURI>                    mURI;
  nsCOMPtr<nsISupports>               mOwner;
  nsCOMPtr<nsISupports>               mSecurityInfo;
  nsCOMPtr<nsIChannel>                mRedirectChannel;
  nsCString                           mContentType;
  nsCString                           mContentCharset;
  PRUint32                            mContentLength;
  PRUint32                            mLoadFlags;
  PRPackedBool                        mQueriedProgressSink;
  PRPackedBool                        mSynthProgressEvents;
  PRPackedBool                        mWasOpened;
  PRPackedBool                        mWaitingOnAsyncRedirect;
  PRPackedBool                        mOpenRedirectChannel;
  PRUint32                            mRedirectFlags;

protected:
  nsCOMPtr<nsILoadGroup>              mLoadGroup;
  nsCOMPtr<nsIStreamListener>         mListener;
  nsCOMPtr<nsISupports>               mListenerContext;
  nsresult                            mStatus;

public:
  // The URI member should be initialized before the channel is used, and then
  // it should never be changed again until the channel is destroyed.
  nsIURI *URI() {
    return mURI;
  }
  void SetURI(nsIURI *uri) {
    NS_ASSERTION(uri, "must specify a non-null URI");
    NS_ASSERTION(!mURI, "must not modify URI");
    NS_ASSERTION(!mOriginalURI, "how did that get set so early?");
    mURI = uri;
    mOriginalURI = uri;
  }
  nsIURI *OriginalURI() {
    return mOriginalURI;
  }


};

#endif /* NenaChannel_h___ */
