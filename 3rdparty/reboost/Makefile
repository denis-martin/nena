
sources := \
	src/buffers/buffer.cpp \
	src/buffers/shared_buffer.cpp \
	src/buffers/message.cpp

objects := $(sources:.cpp=.o)

CPPFLAGS := $(CPPFLAGS) -fPIC -O3

all: $(objects)
	$(AR) rcs libreboost.a $(objects)

clean:
	$(RM) $(objects) libreboost.a

