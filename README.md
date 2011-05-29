# OpenStreetMap History Splitter
This splitter has been developed to split [full-experimental planet dumps](http://wiki.openstreetmap.org/wiki/Planet.osm/full) but it's also possible to split regular [planet dumps](http://wiki.openstreetmap.org/wiki/Planet.osm) with it. It's based on the readers and writers of the [Jochen Topfs](https://github.com/joto) great osmium framework. To be able to work with history files, I created an own [fork](https://github.com/MaZderMind/osmium) of it but all changes are about to be [merged](https://github.com/joto/osmium/pulls) to Jochens master.

This is the tool used to create the extracts hosted on [gwdg](http://ftp5.gwdg.de/pub/misc/openstreetmap/osm-full-history-extracts/).

The splitter currently only supports simple bounding-boxes and the [hardcut-algorithm](https://github.com/MaZderMind/osm-history-splitter/blob/master/hardcut.hpp). Dumps created using that algorithm have the following characteristics:

* ways are cropped at bbox boundaries
* relations contain only members that exist in the extract
* ways and relations are reference-complete
* relations referring to relations that come later in the file are missing this references
* ways that have only one node inside the bbox are missing from the output
* only versions of an object that are inside the bboxes are in the extract, some versions of an object may be missing

Later versions will add other algorithms and polygon-support.


## Build it
In order to compile the splitter, you'll first need the [osmium framework](https://github.com/MaZderMind/osmium) and most of its prequisites:

*   zlib (for PBF support)  
    http://www.zlib.net/  
    Debian/Ubuntu: zlib1g zlib1g-dev  
*   Expat (for parsing XML files)  
    http://expat.sourceforge.net/  
    Debian/Ubuntu: libexpat1 libexpat1-dev  
*   libxml (for writing XML files)  
    http://xmlsoft.org/  
    Debian/Ubuntu:libxml2 libxml2-dev
*   GEOS (for polygon checks)  
    http://trac.osgeo.org/geos/  
    Debian/Ubuntu: libgeos-3.2.0 (older versions might work) libgeos-dev  
*   Google protocol buffers (for PBF support)  
    http://code.google.com/p/protobuf/ (at least Version 2.3.0 needed)  
    Debian/Ubuntu: libprotobuf6 libprotobuf-dev protobuf-compiler  
    Also see http://wiki.openstreetmap.org/wiki/PBF_Format  
*   OSMPBF (for PBF support)  
    https://github.com/MaZderMind/OSM-binary  
    You need to build this first.  

Osmium does not need to be built, it just needs to be referenced in the Makefile. You'll also want the pbf support as .pbf-files can be written between 7 and 20 times faster then .xml.bz2-files. For this you'll need a [version of OSM-binary](https://github.com/MaZderMind/OSM-binary) that supports storing history information.

When you have all prequisites in place, just run *make* to build the splitter.

## Run it
After building the splitter you'll have a single binary: *osm-history-splitter*. The binary takes two parameters and a few options. The splitter is called like that:

    ./osm-history-splitter input.osm.pbf output.config

the splitter reads through input.osm.pbf and splitts it into the extracts listet in output.config. The config-file-format is simple and line-based, it looks like this: 

    woerrstadt.osh.pbf    BBOX    8.1010,49.8303,8.1359,49.8567
    gau-odernheim.osh     BBOX    8.1777,49.7717,8.2056,49.7916

each line consists of three items, separated by spaces:
* the destination path and filename
* the type of extract (currently only BBOX is supported, I'm working on POLY)
* the extract specification (for BBOX it's the boundaries of the bbox, eg. -180,-90,180,90 for the whole world)

If you have any questions just aks at osm@mazdermind.de or via the Github messaging system.

Peter

