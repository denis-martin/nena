#include "nsCOMPtr.h"
#include "nsIConsoleService.h"
#include "NenaInputStream.h"
#include "nsServiceManagerUtils.h"

#include "../../../tmnet/net_errors.h"

//-----------------------------------------------------------------------------
// helper

void
debug_to_console(const char* foo)
{
    nsCOMPtr<nsIConsoleService> aConsoleService =
      do_GetService( "@mozilla.org/consoleservice;1" );

    nsAutoString msg = NS_LITERAL_STRING("nena: ");
    //msg.AssignASCII(foo);
    msg.Append(NS_ConvertUTF8toUTF16(foo));
    aConsoleService->LogStringMessage(msg.get());

}

//-----------------------------------------------------------------------------
// NenaInputStream

NenaInputStream::NenaInputStream(nsACString& uri)
    : mClosed(false)
    , mPos(0)
{
    debug_to_console("load page");
    const char* c_uri;
    uri.BeginReading(&c_uri);
    tmnet_get(&mNenaHandle, c_uri, NULL);
    debug_to_console(c_uri);
    mClosed = false;
    mEndOfStream = false;
}

NenaInputStream::~NenaInputStream()
{
  /* destructor code */
}


//-----------------------------------------------------------------------------
// NenaInputStream::nsISupports

NS_IMPL_ISUPPORTS1(NenaInputStream, nsIInputStream)

/* void close (); */
NS_IMETHODIMP
NenaInputStream::Close()
{
	tmnet_close(mNenaHandle);
    mClosed = true;
    char16_t a = '2';
    return NS_OK;
}

/* unsigned long available (); */
NS_IMETHODIMP
NenaInputStream::Available(PRUint32 *_retval NS_OUTPARAM)
{
    if (mClosed)
        return NS_BASE_STREAM_CLOSED;
    if (mEndOfStream) {
        *_retval = 0;
    } else {
        *_retval = 1024;
    }
    return NS_OK;
}

/* [noscript] unsigned long read (in charPtr aBuf, in unsigned long aCount); */
NS_IMETHODIMP
NenaInputStream::Read(char *aBuf, PRUint32 aCount, PRUint32 *_retval NS_OUTPARAM)
{
    /*
    if (mPos==0) {
        // own memcpy implementation, due to loading errors on other systems
        char *dest = aBuf;
        char *src = "<html><body><h1>It works!</h1></body></html>";
        PRUint32 count = 44;
	while(count--)
	    *dest++ = *src++;
        //memcpy(aBuf, "<html><body><h1>It works!</h1></body></html>", 44);
        *_retval = 44;
        mPos = 44;
        return NS_OK;
    }
    *_retval = 0;
    */
    unsigned long count = aCount;
    //debug_to_console("read");
    int res =  tmnet_read(mNenaHandle, aBuf, &count);
    //debug_to_console("read done");
    //aBuf[aCount] = 0;
    //debug_to_console(aBuf);
    if (res == TMNET_ENDOFSTREAM) {
        *_retval = 0;
	mEndOfStream = true;
    }
    else if (res != TMNET_OK) {
        return NS_ERROR_FAILURE;
    } else {
        *_retval = count;
    }
    return NS_OK;
}

/* [noscript] unsigned long readSegments (in nsWriteSegmentFun aWriter, in voidPtr aClosure, in unsigned long aCount); */
NS_IMETHODIMP
NenaInputStream::ReadSegments(nsWriteSegmentFun aWriter, void *aClosure, PRUint32 aCount, PRUint32 *_retval NS_OUTPARAM)
{
    // will never be implemented, it's not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean isNonBlocking (); */
NS_IMETHODIMP
NenaInputStream::IsNonBlocking(PRBool *_retval NS_OUTPARAM)
{
    *_retval = false;
    return NS_OK;
}

void
NenaInputStream::getMetadata(nsACString& data)
{
	/* TODO
	unsigned int id = nena_id(mNenaHandle);
	nsCAutoString uri;
	uri.AssignLiteral("nhttp:metadata:");
	uri.AppendInt(id, 10);

	const char* c_uri;
	uri.BeginReading(&c_uri);

	//debug_to_console("get metadata");
	phandle hmetadata;
	nena_get(&hmetadata, c_uri, "");

	const unsigned int BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];
	for (unsigned int i = 0; i<BUFFER_SIZE; i++)
		buffer[i] = 0;
	unsigned long count = BUFFER_SIZE - 1;

	nena_read(hmetadata, buffer, &count);
	//debug_to_console("got metadata");
	//debug_to_console(buffer);
	data = buffer;
	nena_close(hmetadata);
	*/
}
