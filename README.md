# OpenStreetMap History Splitter
This splitter has been developed to split [full-experimental planet dumps](http://wiki.openstreetmap.org/wiki/Planet.osm/full) but it's also possible to split regular [planet dumps](http://wiki.openstreetmap.org/wiki/Planet.osm) with it. It's based on the readers and writers of the [Jochen Topfs](https://github.com/joto) great osmium framework.

This is the tool used to create the [hosted extracts](http://osm.personalwerk.de/full-history-extracts/).

The splitter currently supports splitting by bounding-boxes, .poly-files known from osmosis and .osm polygon files (.osm files containing only closed ways).
It implementes two different cutting algorithms (hard- and softcut), which of softcut is the default.

## [softcut-algorithm](https://github.com/MaZderMind/osm-history-splitter/blob/master/softcut.hpp)
* ways stay complete, all used nodes are included (reference-complete)
* relations contains all members, even such that does not exist in the extract (not reference-complete)
* if one version of an object is inside the bbox, all versions are included in the extract (history-complete)
* dual pass processing required

## [hardcut-algorithm](https://github.com/MaZderMind/osm-history-splitter/blob/master/hardcut.hpp)
Dumps created using that algorithm have the following characteristics:

* ways are cropped at bbox boundaries
* relations contain only members that exist in the extract
* ways and relations are reference-complete
* relations referring to relations that come later in the file are missing this references
* ways that have only one node inside the bbox are missing from the output
* only versions of an object that are inside the bboxes are in the extract, some versions of an object may be missing (not history-complete)

## Build it
In order to compile the splitter, you'll first need the [osmium framework](https://github.com/joto/osmium) and most of its prequisites:

*   zlib (for PBF support)
    http://www.zlib.net/
    Debian/Ubuntu: zlib1g-dev
*   Expat (for parsing XML files)
    http://expat.sourceforge.net/
    Debian/Ubuntu: libexpat1-dev
*   libxml (for writing XML files)
    http://xmlsoft.org/
    Debian/Ubuntu:libxml2-dev
*   GEOS (for polygon checks)
    http://trac.osgeo.org/geos/
    Debian/Ubuntu: libgeos-dev libgeos++-dev
*   Google sparsehash
    http://code.google.com/p/google-sparsehash/
    Debian/Ubuntu: libsparsehash-dev
*   Google protocol buffers (for PBF support)
    http://code.google.com/p/protobuf/ (at least Version 2.3.0 needed)
    Debian/Ubuntu: libprotobuf-dev protobuf-compiler
    Also see http://wiki.openstreetmap.org/wiki/PBF_Format
*   OSMPBF (for PBF support)
    https://github.com/scrosby/OSM-binary
    Debian/Ubuntu: libosmpbf-dev

Osmium needs to be present on your system. I recommend to *git clone* osmium directly from [the authors repository](https://github.com/joto/osmium). If you *make install*-ed osmium then osm-history-splitter will locate the osmium headers. You'll also want the pbf support as .pbf-files can be written between 7 and 20 times faster then .xml.bz2-files. For this you'll need a [version of OSM-binary](https://github.com/scrosby/OSM-binary) that supports storing history information.

When you have all prequisites in place, just run *make* to build the splitter.

## Run it
After building the splitter you'll have a single binary: *osm-history-splitter*. The binary takes two parameters and a few options. The splitter is called like that:

    ./osm-history-splitter input.osm.pbf output.config

the splitter reads through input.osm.pbf and splitts it into the extracts listet in output.config. Optionally the following switches are supported:
* --hardcut - enable hardcut mode
* --softcut - enable softcut mode (default)
* --debug - enable debug output

The config-file-format is simple and line-based. Empty lines and lines beginning with # are ignored. A config-file might looks like this:

    woerrstadt.osh.pbf    BBOX    8.1010,49.8303,8.1359,49.8567
    gau-odernheim.osh     OSM     clipbounds/aaa_test/go.osm
    germany.osh           POLY    clipbounds/europe/germany.poly

each line consists of three items, separated by spaces:

* the destination path and filename. The file-extension used specifies the generated file format (.osm, .osh, .osm.bz2, .osh.bz2, .osm.pbf, .osh.pbf)
* the type of extract (BBOX or POLY)
* the extract specification
  * for BBOX: boundaries of the bbox, eg. -180,-90,180,90 for the whole world
  * for OSM:  path to an .osm file from which all closed ways are taken as outlines of a MultiPolygon. Relations are not taken into account, so holes are not possible.
  * for POLY: path to the .poly file

Either both, input and output needs to be history fils or none of them. You can read from an .osh.pbf and write raw-xml .osh files but you can't write to any of the .osm.[pbf|bz2|gz]-type, because these file-types can't store history information. This is true both ways: you can read .osm.bz2 and write .osm.pbf, to give an example, but you can't write to an .osh.pbf because there is no history information in the source file while the destination files is specified as a history file. If you miss this rule, osmium will throw an `Osmium::OSMFile::FileTypeOSMExpected` exception.

The POLY files are in Osmosis' *.poly file format. A huge set of .poly files can be found at [Geofabrik](http://download.geofabrik.de/) (obey the README!) and some tools to work with .poly files are located in the [OpenStreetMap SVN](http://svn.openstreetmap.org/applications/utils/osm-extract/polygons/).

## Big Setups
If you are planning to do a huge number of extracts (something like the [Geofabrik](http://download.geofabrik.de/) does), the split-all-clipbounds.py may be your friend. It scans through the clipbounds directory looking for .poly files (.osm files possible), automatically generates config-files and runs the splitter. It does obey the nesting-rules (ie europe/germany.osm.pbf is generated from europe.osm.pbf) and also ensures the files are created in the correct order.

## Contact
If you have any questions just ask at osm@mazdermind.de or via the Github messaging system.

Peter

