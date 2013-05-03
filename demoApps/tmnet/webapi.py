import os, sys
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, urlsplit, parse_qs
import json
import tmnet
import optparse

from socketserver import ThreadingMixIn
import threading

# httpd handler class
class httpdHandler(BaseHTTPRequestHandler):
    def parseUrl(self, url):
        """Parse URL and return tuple of URI (string) and Requirements (dictionary)"""  
    
        queryStart = url.find("?")
        if queryStart > -1:
            req = parse_qs(url[queryStart+1:])
            return (url[:queryStart], req) 
        else:
            return (url,{})
    
    def writeFile(self, inFile):
        """Write binary file data to reply buffer"""
        if os.path.exists(inFile):
            with open(inFile, "rb") as f:
                bytes = f.read(1024)
                while len(bytes) > 0:
                    self.wfile.write(bytes)

                    bytes = f.read(1024)

    def writeFileChunked(self, inFile):
        """Write binary file to reply buffer as chunks"""
        if os.path.exists(inFile):
            with open(inFile, "rb") as f:
                bytes = f.read(1024)
                count = 0
                while len(bytes) > 0:
                    count = count + 1
                    chunkSize = ("%X\r\n"%(len(bytes)))
                    self.wfile.write(chunkSize.encode("ascii"))
                    self.wfile.write(bytes)
                    self.wfile.write("\r\n".encode("ascii"))
                    bytes = f.read(1024)
                self.wfile.write(("%X\r\n\r\n"%0).encode("ascii"))
                

    def createRequirements(self, req):
        """Check requirements in URL query and return a JSON NENA-Requirement Object as string"""
        newReq = {}
        if "resolution" in req:
            print("resolution")
            newReq["resolution"] = req["resolution"] # is a set now? TODO: change to first element of set?
        if "control" in req:
            print("control")
            newReq["control"] = req["control"] # is a set now? TODO: change to first element of set?
        if "position" in req:
            print("position")
            newReq["position"] = req["position"] # is a set now? TODO: change to first element of set?
        return json.dumps(newReq)
    
    
    def do_GET(self):
        """Handle HTTP GET Request"""


        # Parse URI and extract URI sent down to NENA and Requirements
        uri, req = self.parseUrl(self.path[1:])
        reqJson = self.createRequirements(req)
        #print(reqJson)
        
                
        if uri.startswith("nena"):
            """NENA Request"""
            try:
                handle = tmnet.get(uri.encode("utf8"), reqJson.encode("utf8"))
                self.protocol_version = "HTTP/1.1"
                self.send_response(200)
                self.send_header("Cache-Control", "no-cache")
                #self.send_header("Transfer-Encoding", "chunked") # use with HTTP/1.1 and chunked encoding of the data
                #self.send_header("Content-Type", "") # TODO: implement retrieving of content type from NENA
                self.end_headers()
                while not handle.isEndOfStream() and not self.wfile.closed:
                    nenaData = handle.read(1024)
                    self.wfile.write(nenaData)
                handle.close()
                print("Finished serving WEB-API request.")
            except tmnet.Error as e:
                # An error occured within NENA
                print("TMNET ERROR: %s"%e)
                self.send_error(500, "NENA Error: %s"%e)
                handle.close()
            except IOError as e:
                print("IO ERROR: %s"%e)
                #self.send_error(500, "NENA Error: %s"%e) # not a good idea when the socket causes the exception...
                handle.close()

                
        else:
            print("%s not found"%request)
            self.send_response(404)
    

    def do_POST(self):

        print("not implemented")


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""
    
# run httpd
def run_httpd(server_class=ThreadedHTTPServer, handler_class=BaseHTTPRequestHandler, listen_port=50770):

    try:
        print("Starting httpd...")
        server_address = ('', listen_port)
        httpd = server_class(server_address, handler_class)
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("Received keyboard interrupt. Shutting down...")
        httpd.server_close()


# main
if __name__ == "__main__":
    
    # parse command line parameters
    parser = optparse.OptionParser()
    parser.add_option("--port", dest="port", type="int", default=50770, help="Local Port that the Webserver listens on")
    parser.add_option("--socket", dest="socket", default="/tmp/nena_socket_web", help="Socket for connection to local NENA daemon")
    (options, args) = parser.parse_args()
    
    nenaSocket = options.socket
    port = options.port

    print("Starting API-Webserver/Proxy. Port: %i, Socket: %s"%(port, nenaSocket))


    # Init TMNET API
    tmnet.init()
    tmnet.set_plugin_option("tmnet::nena".encode("utf8"), "ipcsocket".encode("utf8"), nenaSocket.encode("utf8"))
    
    # run httpd
    run_httpd(handler_class=httpdHandler, listen_port=port)

