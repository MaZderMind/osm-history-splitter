#ifndef SPLITTER_CUT_HPP
#define SPLITTER_CUT_HPP

#include <geos/io/WKTWriter.h>
#include <osmium/handler/progress.hpp>
#include <osmium/output.hpp>
#include "geometryreader.hpp"
#include "growing_bitset.hpp"

// information about a single extract
class ExtractInfo {

public:
    enum ExtractMode {
        LOCATOR = 1,
        BOUNDS = 2
    };

    std::string name;
    geos::algorithm::locate::IndexedPointInAreaLocator *locator;
    Osmium::OSM::Bounds bounds;
    Osmium::Output::Base *writer;
    ExtractMode mode;

    ExtractInfo(std::string name) : locator(NULL), writer(NULL) {
        this->name = name;
    }

    ~ExtractInfo() {
        if(locator) delete locator;
        if(writer) delete writer;
    }

    bool contains(const shared_ptr<Osmium::OSM::Node const>& node) {
        if(mode == BOUNDS) {
            return
                (node->lon() > bounds.bottom_left().lon()) &&
                (node->lat() > bounds.bottom_left().lat()) &&
                (node->lon() < bounds.top_right().lon()) &&
                (node->lat() < bounds.top_right().lat());
        }
        else if(mode == LOCATOR) {
            // BOUNDARY 1
            // EXTERIOR 2
            // INTERIOR 0

            geos::geom::Coordinate c = geos::geom::Coordinate(node->lon(), node->lat(), DoubleNotANumber);
            return (0 == locator->locate(&c));
        }

        return false;
    }
};

// information about the cutting algorithm
template <class TExtractInfo>
class CutInfo {

protected:
    ~CutInfo() {
        for(int i=0, l = extracts.size(); i<l; i++) {
            extracts[i]->writer->final();
            delete extracts[i];
        }
    }

public:
    std::vector<TExtractInfo*> extracts;


    TExtractInfo *addExtract(std::string name, double minlon, double minlat, double maxlon, double maxlat) {
        fprintf(stderr, "opening writer for %s\n", name.c_str());
        Osmium::OSMFile outfile(name);
        Osmium::Output::Base *writer = Osmium::Output::Factory::instance().create_output(outfile);

        const Osmium::OSM::Position min(minlat, minlon);
        const Osmium::OSM::Position max(maxlat, maxlon);

        Osmium::OSM::Bounds bounds;
        bounds.extend(min).extend(max);

        Osmium::OSM::Meta meta(bounds);
        writer->init(meta);

        TExtractInfo *ex = new TExtractInfo(name);
        ex->writer = writer;
        ex->bounds = bounds;
        ex->mode = ExtractInfo::BOUNDS;

        extracts.push_back(ex);
        return ex;
    }

    TExtractInfo *addExtract(std::string name, geos::geom::Geometry *poly) {
        fprintf(stderr, "opening writer for %s\n", name.c_str());
        Osmium::OSMFile outfile(name);
        Osmium::Output::Base *writer = Osmium::Output::Factory::instance().create_output(outfile);

        const geos::geom::Envelope *env = poly->getEnvelopeInternal();
        const Osmium::OSM::Position min(env->getMinX(), env->getMinY());
        const Osmium::OSM::Position max(env->getMaxX(), env->getMaxY());

        Osmium::OSM::Bounds bounds;
        bounds.extend(min).extend(max);

        Osmium::OSM::Meta meta(bounds);
        writer->init(meta);

        TExtractInfo *ex = new TExtractInfo(name);
        ex->writer = writer;
        ex->locator = new geos::algorithm::locate::IndexedPointInAreaLocator(*poly);
        ex->mode = ExtractInfo::LOCATOR;

        Osmium::Geometry::geos_geometry_factory()->destroyGeometry(poly);

        extracts.push_back(ex);
        return ex;
    }
};

template <class TCutInfo>
class Cut : public Osmium::Handler::Base {

protected:

    Osmium::Handler::Progress pg;
    TCutInfo *info;

public:

    bool debug;
    Cut(TCutInfo *info) : info(info) {}
};

#endif // SPLITTER_CUT_HPP

