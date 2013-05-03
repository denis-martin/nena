MODULE		= nenaprotocols

NENAAPI_PATH   ?= $(CURDIR)/../../../tmnet
NENAAPI_LIB    ?= libtmnet.so

GECKO_SDK_PATH ?= $(HOME)/xulrunner-sdk

XPIDL_HEADER   ?= python $(GECKO_SDK_PATH)/sdk/bin/header.py
XPIDL_TYPELIB  ?= python $(GECKO_SDK_PATH)/sdk/bin/typelib.py

CXX	       ?= c++

XPIDLSRCS	= \
		INenaProtocolHandler.idl \
		$(NULL)

CPPSRCS		= \
		NenaInputStream.cpp \
		NenaChannel.cpp \
		NenaProtocolHandler.cpp \
		NenaProtocolsModule.cpp \
		$(NULL)

CPPFLAGS +=     -fno-rtti		\
                -fno-exceptions		\
                -fshort-wchar		\
		-fPIC			\
		-std=c++0x		\
		$(NULL)

# GCC only define which allows us to not have to #include mozilla-config
# in every .cpp file.  If your not using GCC remove this line and add
# #include "mozilla-config.h" to each of your .cpp files.
GECKO_CONFIG_INCLUDE = -include mozilla-config.h

GECKO_DEFINES  = -DMOZILLA_STRICT_API

GECKO_INCLUDES = -I$(GECKO_SDK_PATH)                    \
                 -I$(GECKO_SDK_PATH)/idl                \
                 -I$(GECKO_SDK_PATH)/include            \
                 -I$(GECKO_SDK_PATH)/xpcom/include      \
                 -I$(GECKO_SDK_PATH)/nspr/include       \
                 -I$(GECKO_SDK_PATH)/string/include     \
                 -I$(GECKO_SDK_PATH)/embedstring/include

GECKO_LDFLAGS =  -L$(GECKO_SDK_PATH)/bin \
		 -L$(GECKO_SDK_PATH)/lib \
		 -lxpcomglue_s	\
		 -lxpcom        \
                 -lnspr4	\
                 -lplds4	\
		 -lxul		\
		 -l:$(NENAAPI_PATH)/$(NENAAPI_LIB)	\
		 -shared	\
		 $(NULL)

%.h: %.idl
	$(XPIDL_HEADER) $(GECKO_INCLUDES) -o $(<:%.idl=%.h) $<

%.xpt: %.idl
	$(XPIDL_TYPELIB) $(GECKO_INCLUDES) -o $(<:%.idl=%.xpt) $<

%.o: %.cpp Makefile
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $(GECKO_CONFIG_INCLUDE) $(GECKO_DEFINES) $(GECKO_INCLUDES) $<

$(MODULE).so: $(XPIDLSRCS:%.idl=%.h) $(XPIDLSRCS:%.idl=%.xpt) $(CPPSRCS:%.cpp=%.o)
	$(CXX) -o $@ -Wl,-soname=$(MODULE).so $(CPPSRCS:%.cpp=%.o) $(GECKO_LDFLAGS)
	chmod +x $@

build: $(MODULE).so
all: $(MODULE).so

clean:
	rm -f $(MODULE).so $(XPIDLSRCS:%.idl=%.xpt) *.o
