#------------------------------------------------------------------------------
#
#  osm history splitter makefile
#
#------------------------------------------------------------------------------

CXX = g++

CXXFLAGS = -g
#CXXFLAGS = -O3

CXXFLAGS += -std=c++0x -Wall -Wextra -pedantic
#CXXFLAGS += -Wredundant-decls -Wdisabled-optimization
#CXXFLAGS += -Wpadded -Winline

# uncomment this if you want information on how long it took to build the multipolygons
#CXXFLAGS += -DOSMIUM_WITH_MULTIPOLYGON_PROFILING

CXXFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I../../osmium/include
CXXFLAGS += `xml2-config --cflags`

LDFLAGS = -L/usr/local/lib -lexpat -lpthread
LDFLAGS += `xml2-config --libs`

LIB_PROTOBUF = -lz -lprotobuf-lite -losmpbf

.PHONY: all clean install

all: splitter

splitter: splitter.cpp hardcut.hpp softcut.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LIB_PROTOBUF)

install:
	install -m 755 -g root -o root -d $(DESTDIR)/usr/bin
	install -m 755 -g root -o root splitter $(DESTDIR)/usr/bin/splitter

clean:
	rm -f *.o core splitter

