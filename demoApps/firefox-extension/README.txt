==About==

Firefox plugin for tmnet


==Compile==

You need the xulrunner-sdk from ftp://ftp.mozilla.org/pub/mozilla.org/xulrunner/releases/

Currently version 9.0.1 is supported.

In ./Makefile, you need to adjust the following parameters:

  GECKO_SDK_PATH - pointing to your xulrunner-sdk directory

In components/src/Makefile:

  NENAAPI_PATH - path to NENA-API library
  NENAAPI_LIB - filename of library


==Testing==

In Error console:
Components.classes["@mozilla.org/network/protocol;1?name=nena"].getService(Components.interfaces.nsIProtocolHandler).newURI("nena:test", null, null)

Components.classes["@mozilla.org/network/protocol;1?name=nena"].getService(Components.interfaces.nsIProtocolHandler).newChannel(Components.classes["@mozilla.org/network/protocol;1?name=nena"].getService(Components.interfaces.nsIProtocolHandler).newURI("nena:test", null, null)).contentType

==Links==

* http://www.nexgenmedia.net/docs/protocol/
* http://www-archive.mozilla.org/projects/netlib/new-handler.html
* https://developer.mozilla.org/en/nsIProtocolHandler
* https://developer.mozilla.org/en/nsIChannel
* http://kb.mozillazine.org/Dev_:_Extending_the_Chrome_Protocol

* https://developer.mozilla.org/en/Creating_XPCOM_Components%3aSetting_up_the_Gecko_SDK
* https://developer.mozilla.org/En/Troubleshooting_XPCOM_components_registration
