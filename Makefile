#------------------------------------------------------------------------------
#
#  osm history splitter makefile
#
#------------------------------------------------------------------------------

PREFIX ?= /usr

#CXX = g++
CXX = clang++

CXXFLAGS = -g -O3 -Wall -Wextra -pedantic
CXXFLAGS += `getconf LFS_CFLAGS`
#CXXFLAGS += -Wredundant-decls -Wdisabled-optimization
#CXXFLAGS += -Wpadded -Winline

# compile & link against libxml to have xml writing support
CXXFLAGS += -DOSMIUM_WITH_OUTPUT_OSM_XML
CXXFLAGS += `xml2-config --cflags`
LDFLAGS = -L/usr/local/lib -lexpat -lpthread
LDFLAGS += `xml2-config --libs`

# compile &  link against libs needed for protobuf reading and writing
LDFLAGS += -lz -lprotobuf-lite -losmpbf

# compile &  link against geos for multipolygon extracts
CXXFLAGS += `geos-config --cflags`
CXXFLAGS += -DOSMIUM_WITH_GEOS
LDFLAGS += `geos-config --libs`

.PHONY: all clean install

all: osm-history-splitter

osm-history-splitter: splitter.cpp hardcut.hpp softcut.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

install: osm-history-splitter
	install -m 755 -g root -o root -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 -g root -o root osm-history-splitter $(DESTDIR)$(PREFIX)/bin/osm-history-splitter

clean:
	rm -f *.o core osm-history-splitter

