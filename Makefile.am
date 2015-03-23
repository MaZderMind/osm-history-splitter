bin_PROGRAMS=osm_history_splitter
osm_history_splitter_SOURCES=splitter.cpp hardcut.hpp softcut.hpp

LDADD= $(XML_LIBS) -lexpat  -losmpbf -lz -lprotobuf  $(GEOS_LIBS)
#@PROTOBUF_LITE_LIBS@ @PROTOBUF_LIBS@
#  uncomment the following if osm_history_splitter requires the math library
#osm_history_splitter_LDADD=-lm
AM_CPPFLAGS = $(XML_CPPFLAGS) $(GEOS_CFLAGS)


#osm_history_splitter_LDFLAGS= 

EXTRA_DIST=osm_history_splitter.lsm.in #NAME.texinfo osm_history_splitter.spec.in

#  if you write a self-test script named `chk', uncomment the
#  following and add `chk' to the EXTRA_DIST list
#TESTS=chk
