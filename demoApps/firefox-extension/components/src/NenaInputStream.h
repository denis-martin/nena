#ifndef NenaInputStream_h___
#define NenaInputStream_h___

#include "nsStringAPI.h"
#include "nsIInputStream.h"

#include "../../../tmnet/net_c.h"


class NenaInputStream : public nsIInputStream
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM

  NenaInputStream(nsACString& uri);
  void getMetadata(nsACString& data);

private:
  ~NenaInputStream();

protected:
  bool     mClosed;
  bool     mEndOfStream;
  int      mPos;
  tmnet_pnhandle mNenaHandle;

};

#endif /* NenaInputStream_h___ */
