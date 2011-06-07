#ifndef SPLITTER_CUT_HPP
#define SPLITTER_CUT_HPP

#include <geos/io/WKTWriter.h>
#include <osmium/handler/progress.hpp>
#include <osmium/utils/geometryreader.hpp>

class ExtractInfo {

public:
    std::string name;
    geos::algorithm::locate::IndexedPointInAreaLocator *locator;
    Osmium::Output::OSM::Base *writer;

    // the initial size of the id-trackers could be 0, because the vectors
    // are flexible, but providing here an estimation of the max. number of nodes
    // and ways gives the tool a "fail first" behaviour in the case of not enough memory 
    // (better fail in init phase, not after 5 hours of processing)
    static const unsigned int est_max_node_id =   1400000000;
    static const unsigned int est_max_way_id =     130000000;
    static const unsigned int est_max_relation_id =  1000000;

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

template <class TExtractInfo> class Cut : public Osmium::Handler::Base {

protected:

    Osmium::Handler::Progress *pg;

    std::vector<TExtractInfo*> extracts;

    ~Cut() {
        for(int i=0, l = extracts.size(); i<l; i++) {
            extracts[i]->writer->write_final();
            delete extracts[i]->writer;
            delete extracts[i]->locator;
            delete extracts[i];
        }
        delete pg;
    }

public:

    bool debug;

    Cut() {
        pg = new Osmium::Handler::Progress();
    }

    TExtractInfo *addExtract(std::string name, geos::geom::Geometry *poly) {
        fprintf(stderr, "opening writer for %s\n", name.c_str());
        Osmium::OSMFile* outfile = new Osmium::OSMFile(name);
        Osmium::Output::OSM::Base *writer = outfile->create_output_file();
        delete outfile;

        writer->write_init();
        const geos::geom::Envelope *env = poly->getEnvelopeInternal();
        writer->write_bounds(env->getMinX(), env->getMinY(), env->getMaxX(), env->getMaxY());

        TExtractInfo *ex = new TExtractInfo(name);
        ex->writer = writer;

        ex->locator = new geos::algorithm::locate::IndexedPointInAreaLocator(*poly);
        delete poly;

        extracts.push_back(ex);
        return ex;
    }
};

#endif // SPLITTER_CUT_HPP

