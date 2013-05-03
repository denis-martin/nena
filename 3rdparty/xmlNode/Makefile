#
# xmlNode test
#

HEADERS = xmlNode.h

SOURCES = xmlNode.cpp test.cpp

TARGET = test

OBJECTS = $(SOURCES:.cpp=.o)

all: test

test: $(HEADERS) $(SOURCES) $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS)

clean:
	$(RM) $(TARGET) $(OBJECTS)
