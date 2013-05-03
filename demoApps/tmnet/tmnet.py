"""
Python Wrapper for the TMNET C-API

Usage/quick start:
init() then use connect(), put(), get(), or bind() to create a new handle.
The handle's methods can then be used to read()/write() data etc. (see methods
descriptions).


========
C-API: (net_c.h)
========

struct tmnet_nhandle;
typedef struct tmnet_nhandle* tmnet_pnhandle;
typedef char* tmnet_req_t;

int tmnet_init();

int tmnet_bind(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
int tmnet_wait(tmnet_pnhandle handle);
int tmnet_accept(tmnet_pnhandle handle, tmnet_pnhandle* inc);
int tmnet_get_property(tmnet_pnhandle handle, const char* key, char** value);

int tmnet_connect(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);

int tmnet_read(tmnet_pnhandle handle, char* buf, size_t* size);
int tmnet_write(tmnet_pnhandle handle, const char* buf, size_t size);

int tmnet_readmsg(tmnet_pnhandle handle, char* buf, size_t* size);
int tmnet_writemsg(tmnet_pnhandle handle, const char* buf, size_t size);

int tmnet_close(tmnet_pnhandle handle);

int tmnet_set_option(tmnet_pnhandle handle, const char* name, const char* value);
int tmnet_set_plugin_option(const char* plugin, const char* name, const char* value);

int tmnet_get(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
int tmnet_put(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);


======
Notes:
======

A simple void pointer is used for tmnet_pnhandle in the wrapper implementation
because the tmnet_nhandle struct is not really used "above" the API
"""

import ctypes

import os
dll = "libtmnet.so" # File name of the C-API library
path = os.path.dirname(os.path.realpath(__file__))
dllpath = "%s/%s"%(path, dll)

#capi = ctypes.CDLL("./libtmnet.so")
capi = ctypes.CDLL(dllpath) # Load the C-API library


####################
# init the library #
####################

def init():
    """Initialize API.
    
    C-API:
    int tmnet_init();
    """
    result = capi.tmnet_init()
    if result != 0:
        raise Error(result)
    return result

def set_plugin_option(plugin, name, value):
    """Set plugin option of the C-API. E.g., used to "select" local NENA instance.
    
    C-API:
    int tmnet_set_plugin_option(const char* plugin, const char* name, const char* value);
    """
    result = capi.tmnet_set_plugin_option(ctypes.c_char_p(plugin), ctypes.c_char_p(name), ctypes.c_char_p(value))
    if result != 0:
        raise Error(result)
    return result


#########################################
# methods to initialize/return a handle #
#########################################

def connect(uri, req):
    """Creates a new communication handle for a read/write communication
    process. Connects to URI uri with requirements req and returns a new handle
    
    C-API:
    int tmnet_connect(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
    """
    handle = ctypes.c_void_p(None)
    result = capi.tmnet_connect(ctypes.byref(handle), ctypes.c_char_p(uri), ctypes.c_char_p(req))
    if result != 0:
        raise Error(result)
    nhandle = Handle(handle)
    return nhandle
    
def put(uri, req):
    """Creates a new communication handle for a write only communication
    process. Connects to URI uri with requirements req and returns a new
    handle.
    
    C-API:
    int tmnet_put(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
    """
    handle = ctypes.c_void_p(None)
    result = capi.tmnet_put(ctypes.byref(handle), ctypes.c_char_p(uri), ctypes.c_char_p(req))
    if result != 0:
        raise Error(result)
    nhandle = Handle(handle)
    return nhandle

   
def get(uri, req):
    """Creates a new communication handle for a read only communication
    process. Connects to URI uri with requirements req and returns a new
    handle.
    
    C-API:
    int tmnet_get(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
    """
    handle = ctypes.c_void_p(None)
    result = capi.tmnet_get(ctypes.byref(handle), ctypes.c_char_p(uri), ctypes.c_char_p(req))
    if result != 0:
        raise Error(result)
    nhandle = Handle(handle)
    return nhandle

def bind(uri, req):
    """Bind to URI uri with requirements req. Returns a handle from which new
    communication requests (i.e. handles) can be accepted via accept().
    
    C-API:
    int tmnet_bind(tmnet_pnhandle* handle, const char* uri, const tmnet_req_t req);
    """
    handle = ctypes.c_void_p(None)
    result = capi.tmnet_bind(ctypes.byref(handle), ctypes.c_char_p(uri),ctypes.c_char_p(req))
    if result != 0:
        raise Error(result)
    nhandle = Handle(handle)
    return nhandle
    
    
################
# Handle Class #
################

class Handle():
    """Class for the handle that is used to identify communication processes.
    
    Attributes:
    handle: contains the handle
    type: type of the handle: read-only r, write-only w, read/write rw, listen-only l - FOR FUTURE USE?
    """
    def __init__(self, handle = ctypes.c_void_p(None), type="rw"):
        self.handle = handle
        self.type = type
    
    
    def accept(self):
        """Accepts a new communication request from the handle (must have been
        created with bind()!). Returns a new "regular" handle for a new
        communication request.
        
        C-API:
        int tmnet_accept(tmnet_pnhandle handle, tmnet_pnhandle* inc);
        """
        inc = ctypes.c_void_p(None)
        result = capi.tmnet_accept(self.handle, ctypes.byref(inc))
        if result != 0:
            raise Error(result)
        nhandle = Handle(inc)
        return nhandle

    def read(self, size):
        """Reads data from the handle into a buffer of length size. Returns the
        buffer with the read data.
        
        C-API:
        int tmnet_read(tmnet_pnhandle handle, char* buf, size_t* size);
        """
        buffer = ctypes.create_string_buffer(size)
        recvsize = ctypes.c_size_t(size)
        result = capi.tmnet_read(self.handle, buffer, ctypes.byref(recvsize))
        if result == 0 or result == 0x300:
            return buffer.raw[:recvsize.value]
        else:
            raise Error(result)
        
    
    def write(self, buffer):
        """Writes data to the handle. Reads data from the supplied buffer.
        
        C-API:
        int tmnet_write(tmnet_pnhandle handle, const char* buf, size_t size);
        """
        nbuffer = ctypes.create_string_buffer(buffer)
        result = capi.tmnet_write(self.handle, nbuffer, ctypes.c_size_t(len(buffer)))
        if result != 0:
            raise Error(result)
        return result
    
#    def readmsg(self, size):
#        """
#        C-API:
#        int tmnet_readmsg(tmnet_pnhandle handle, char* buf, size_t* size);
#        """
#        pass
    
#    def writemsg(self):
#        """
#        C-API:
#        int tmnet_writemsg(tmnet_pnhandle handle, const char* buf, size_t size);
#        """
#        pass
            
    def wait(self):
        """Waits on changes of the handle, e.g., new data/event arrived and
        then returns.
        
        C-API:
        int tmnet_wait(tmnet_pnhandle handle);
        """
        result = capi.tmnet_wait(self.handle)
        if result != 0:
            raise Error(result)
        return result
    
    def get_property(self, key):
        """Query property of the handle identified by key. Returns the
        resulting value.
        
        C-API:
        int tmnet_get_property(tmnet_pnhandle handle, const char* key, char** value);
        """
        value = ctypes.c_char_p()
        result = capi.tmnet_get_property(self.handle, ctypes.c_char_p(key), ctypes.byref(value))
        if result != 0:
            raise Error(result)
        retval = value.value
        capi.tmnet_free_property(value)
        #return value.value
        return retval
    
    def close(self):
        """Close communication handle.
        
        C-API:
        int tmnet_close(tmnet_pnhandle handle);
        """
        result = capi.tmnet_close(self.handle)
        if result != 0:
            raise Error(result)
        return result
    
    def isEndOfStream(self):
        """Return true if end of stream has been reached"""
        result = capi.tmnet_isEndOfStream(self.handle)
        #print("tmnet debug: end of stream: %i"%result)
        if result != 0:
            return True
        else:
            return False

##########
# Errors #
##########

class Error(Exception):
    """Exception used to pass C-API errors to the user of this Python Wrapper.
    Error codes from the C-API are used as parameter and then are converted to
    the exception's (human readable) string.
    
    C-API Error Codes:
    #define TMNET_OK                0x000
    #define TMNET_INVALID_PARAMETER    0x001
    #define TMNET_UNSUPPORTED        0x100

    // plugin related
    #define TMNET_PLUGIN_FAILED                    0x200
    #define TMNET_SCHEME_ALREADY_REGISTERED        0x201
    #define TMNET_SYSTEM_ERROR                    0x202

    // flow related
    #define TMNET_ENDOFSTREAM        0x300
    """
    error = {
             0x000: "OK",
             0x001: "INVALID_PARAMETER",
             0x100: "UNSUPPORTED",
             0x200: "PLUGIN_FAILED",
             0x201: "SCHEME_ALREADY_REGISTERED",
             0x202: "SYSTEM_ERROR",
             0x300: "ENDOFSTREAM"
    }
    
    def __init__(self, code):
        self.code = code
                
    def __str__(self):
        if self.code in self.__class__.error:
            return repr("ERROR %i: %s" %(self.code, self.__class__.error[self.code]))
        else:        
            return repr("ERROR %i: Unknown TMNET error."%self.code)

#############
# Test code #
#############

# main
if __name__ == "__main__":
    # This is the client side of a Ping/Echo-Service as test/example code
    init()
    con = connect("app://localhost.node02/ping_tmnet", "")
    con.write("PING")
    reply = con.read(1500) # expects/reads a reply from the server side
    print(reply)

