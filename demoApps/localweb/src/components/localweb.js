/**
 * License: Public Domain
 */

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const nsIProtocolHandler = Ci.nsIProtocolHandler;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
console = Components.classes["@mozilla.org/consoleservice;1"].getService(Components.interfaces.nsIConsoleService);

var protocol_scheme = "LWSCHEME";
var protocol_destination = "http://LWHOST";
var component_id = '{LWUUID}';

function RewriteUri(LocalWebResource)
{
	var resource = LocalWebResource; //.split(":")[1];
	resource = protocol_destination + "/" + resource;
	return resource; 
}

function LocalWeb()
{

}

LocalWeb.prototype = {
	scheme: protocol_scheme,
	protocolFlags: 
		nsIProtocolHandler.URI_NOAUTH |
		nsIProtocolHandler.URI_LOADABLE_BY_ANYONE,

	newURI: function(aSpec, aOriginCharset, aBaseURI)
	{
		var uri = Cc["@mozilla.org/network/standard-url;1"].createInstance(Ci.nsIURI);
/*		console.logStringMessage("newURI: aSpec: " + aSpec + "\n" +
			"aBaseURI: " + ((aBaseURI == null) ? "null" : aBaseURI.spec) + "\n" +
			"aBaseURI.resolve: " + ((aBaseURI === null) ? aSpec : aBaseURI.resolve(aSpec))); */
		uri.spec = ((aBaseURI === null) ? aSpec : aBaseURI.resolve(aSpec));
		return uri;
	},

	newChannel: function(aURI)
	{
		var ioservice = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
		var LocalWebResource = aURI.spec;
		var rewrittenUri = RewriteUri(LocalWebResource);
		var uri = ioservice.newURI(rewrittenUri, null, null);
		var channel = ioservice.newChannelFromURI(uri, null).QueryInterface(Ci.nsIHttpChannel);
		channel.originalURI = aURI;
		return channel;
	},

	classDescription: "LocalWeb Protocol Handler (" + protocol_scheme + ":-version)",
	contractID: "@mozilla.org/network/protocol;1?name=" + protocol_scheme,
	classID: Components.ID(component_id),
	QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler])
}

if (XPCOMUtils.generateNSGetFactory)
	var NSGetFactory = XPCOMUtils.generateNSGetFactory([LocalWeb]);
else
	var NSGetModule = XPCOMUtils.generateNSGetModule([LocalWeb]);

