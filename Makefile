#------------------------------------------------------------------------------
#
#  osm history splitter makefile
#
#------------------------------------------------------------------------------

CXX = g++

CXXFLAGS = -g -O3 -std=c++0x -Wall -Wextra -pedantic
#CXXFLAGS += -Wredundant-decls -Wdisabled-optimization
#CXXFLAGS += -Wpadded -Winline

# compile & link against libxml to have xml writing support
CXXFLAGS += -DOSMIUM_WITH_OUTPUT_OSM_XML -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I../osmium/include
CXXFLAGS += `xml2-config --cflags`
LDFLAGS = -L/usr/local/lib -lexpat -lpthread
LDFLAGS += `xml2-config --libs`

# link against libs needed for protobuf reading and writing
LDFLAGS += -lz -lprotobuf-lite -losmpbf



.PHONY: all clean install

all: osm-history-splitter

osm-history-splitter: splitter.cpp hardcut.hpp softcut.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

install:
	install -m 755 -g root -o root -d $(DESTDIR)/usr/bin
	install -m 755 -g root -o root osm-history-splitter $(DESTDIR)/usr/bin/osm-history-splitter

clean:
	rm -f *.o core osm-history-splitter

