/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM INenaProtocolHandler.idl
 */

#ifndef __gen_INenaProtocolHandler_h__
#define __gen_INenaProtocolHandler_h__


#ifndef __gen_nsIProtocolHandler_h__
#include "nsIProtocolHandler.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    INenaProtocolHandler */
#define INENAPROTOCOLHANDLER_IID_STR "71dbf9fa-aee9-427e-8004-641fe041ac7a"

#define INENAPROTOCOLHANDLER_IID \
  {0x71dbf9fa, 0xaee9, 0x427e, \
    { 0x80, 0x04, 0x64, 0x1f, 0xe0, 0x41, 0xac, 0x7a }}

class NS_NO_VTABLE NS_SCRIPTABLE INenaProtocolHandler : public nsIProtocolHandler {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(INENAPROTOCOLHANDLER_IID)

};

  NS_DEFINE_STATIC_IID_ACCESSOR(INenaProtocolHandler, INENAPROTOCOLHANDLER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_INENAPROTOCOLHANDLER \
  /* no methods! */

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_INENAPROTOCOLHANDLER(_to) \
  /* no methods! */

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_INENAPROTOCOLHANDLER(_to) \
  /* no methods! */

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class _MYCLASS_ : public INenaProtocolHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INENAPROTOCOLHANDLER

  _MYCLASS_();

private:
  ~_MYCLASS_();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(_MYCLASS_, INenaProtocolHandler)

_MYCLASS_::_MYCLASS_()
{
  /* member initializers and constructor code */
}

_MYCLASS_::~_MYCLASS_()
{
  /* destructor code */
}

/* End of implementation class template. */
#endif


#endif /* __gen_INenaProtocolHandler_h__ */
