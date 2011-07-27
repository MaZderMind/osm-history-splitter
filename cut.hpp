#ifndef SPLITTER_CUT_HPP
#define SPLITTER_CUT_HPP

#include <geos/io/WKTWriter.h>
#include <osmium/handler/progress.hpp>
#include <osmium/utils/geometryreader.hpp>

// information about a single extract
class ExtractInfo {

public:
    std::string name;
    geos::algorithm::locate::IndexedPointInAreaLocator *locator;
    Osmium::Output::Base *writer;

    // the initial size of the id-trackers could be 0, because the vectors
    // are flexible, but providing here an estimation of the max. number of nodes
    // and ways gives the tool a "fail first" behaviour in the case of not enough memory 
    // (better fail in init phase, not after 5 hours of processing)
    static const unsigned int est_max_node_id =   1400000000;
    static const unsigned int est_max_way_id =     130000000;
    static const unsigned int est_max_relation_id =  1500000;

    ExtractInfo(std::string name) {
        this->name = name;
    }

    bool contains(Osmium::OSM::Node *node) {
        // BOUNDARY 1
        // EXTERIOR 2
        // INTERIOR 0

        geos::geom::Coordinate c = geos::geom::Coordinate(node->get_lon(), node->get_lat(), DoubleNotANumber);
        return (0 == locator->locate(&c));
    }
};

// information about the cutting algorithm
template <class TExtractInfo>
class CutInfo {

protected:
    ~CutInfo() {
        for(int i=0, l = extracts.size(); i<l; i++) {
            extracts[i]->writer->final();
            delete extracts[i]->writer;
            delete extracts[i]->locator;
            delete extracts[i];
        }
    }

public:
    std::vector<TExtractInfo*> extracts;

    TExtractInfo *addExtract(std::string name, geos::geom::Geometry *poly) {
        fprintf(stderr, "opening writer for %s\n", name.c_str());
        Osmium::OSMFile outfile(name);
        Osmium::Output::Base *writer = outfile.create_output_file();

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

