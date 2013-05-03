import os, sys
lib_path = os.path.abspath('../tmnet')
sys.path.append(lib_path)
import tmnet
import json
import time
#import argparse #not in python2 versions below 2.7 or python3 below 3.2
import optparse

import threading

class NenaBinding():
    """Helper Class for URIs and Handles the server is bound to/associated with"""
    def NenaBinding(self):
        self.uri = None
        self.listenHandle = None
    
class RequestHandlerThread(threading.Thread):
    """Request Handler Thread"""
    
    def __init__(self, handle, binding, dir):
        """Stores handle the request uses"""
        self.handle = handle
        self.binding = binding
        self.dir = dir
        
        threading.Thread.__init__ ( self )
    
    def run(self):
        """Processes the request"""
        
        handle = self.handle
        binding = self.binding
        
        # get requested uri
        #requestUri = handle.getProperty("requestURI")
        reqJson = handle.get_property("requirements")
        print("Requirements: %s"%reqJson)
        req = json.loads(reqJson)
        requestedUri = req["uri"]
        print("Requested URI: %s"%requestedUri)
        # extract requested file from uri
        requestedFile = requestedUri[len(binding.uri)+1:] # strip binded URI from requested URI to get the file name
        print("Requested File: %s"%requestedFile)
        # write file to handle
        try:
            self.writeFile(handle, requestedFile)
        except tmnet.Error as e:
            # error using TMNET!
            print("TMNET ERROR getting/returning NENA data: %s"%e)
        #handle.write("""{"super": "fileServer"}""".encode("ascii"))
        # close handle
        try:
            handle.close()
        except tmnet.Error as e:
            # error using TMNET!
            print("TMNET ERROR closing handle: %s"%e)
        #time.sleep(3)
        
    def writeFile(self, handle, requestedFile):
        """Write requested file to handle""" 
        
        requestedFile = self.dir + requestedFile # look for files relative to server's dir
        
        if os.path.exists(requestedFile):
            with open(requestedFile, "rb") as f:
                bytes = f.read(1024)
                while len(bytes) > 0:
                    handle.write(bytes)
                    bytes = f.read(1024)
        else:
            print("FileServer: File not found!")


class NenaServer():
    """NENA File Server Class"""
    def __init__(self, dir=""):
        # NENA Server attributes
        self.uris = None # URIs/Prefixes that the server serves
        #self.binded = False # are we binded to the URIs yet?
        self.bindings = list() # what the server is binded to
        self.dir=dir        
        return
    
    def serveForever(self):
        """Main server loop"""
        while True:
        
            # wait for next user request. ONLY 1 URI/HANDLE SUPPORTED! TODO: extend...
            binding = self.bindings[0]
            
            try:
                listenHandle = binding.listenHandle
                listenHandle.wait()
                # get new user request
                handle = listenHandle.accept()
            except tmnet.Error as e:
                # error using TMNET!
                print("TMNET ERROR on listen handle: %s. Sleeping 1 second and returning to main loop"%e)
                time.sleep(1)
                continue
                
            # start new thread that serves the request
            RequestHandlerThread(handle, binding, self.dir).start()

        return
    
    def serveUris(self, uris):
        """Serve the following set of URI or URI prefix strings"""
        self.uris = uris
        if len(self.bindings) > 0:
            # we are binded to other URIs, close handle and re-bind!
            for b in self.bindings:
                b.listenHandle.close()
        self.bindings = list()
        for uri in uris:
            # bind to uri and add it to the listenHandles list
            b = NenaBinding()
            b.uri = uri
            b.listenHandle = tmnet.bind(uri, "")
            self.bindings.append(b)
        return


# main
if __name__ == "__main__":
    # parse command line arguments
    # Doesn't work with python 2.6 or python 3.1
    #parser = argparse.ArgumentParser()
    #parser.add_argument('--socket', default="/tmp/nena_socket_web")
    #args = parser.parse_args()
    #nenaSocket = args.socket
    
    # parse command line parameters
    parser = optparse.OptionParser()
    parser.add_option("--dir", dest="dir", default="", help="Directory that is served by the fileServer")
    parser.add_option("--uri", dest="uri", default="app://localhost.node02/file", help="URI that the fileServer is binded to")
    parser.add_option("--socket", dest="socket", default="/tmp/nena_socket_web", help="Socket for connection to local NENA daemon")
    (options, args) = parser.parse_args()
    
    nenaSocket = options.socket
    serverUri = options.uri
    serverDir = options.dir
    
    print("Starting fileServer. Dir: %s, URI: %s, Socket: %s"%(serverDir, serverUri, nenaSocket))
    
    # Initialize TMNET/NENA API
    tmnet.init()
    tmnet.set_plugin_option("tmnet::nena", "ipcsocket", nenaSocket)

    # start server
    server = NenaServer(serverDir)
    server.serveUris((serverUri,)) #only 1 URI supported at the moment!
    try:
        server.serveForever()
    except KeyboardInterrupt:
        print("Received Keyboard Interrupt. Closing down...")
        