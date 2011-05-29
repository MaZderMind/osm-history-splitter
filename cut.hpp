#ifndef SPLITTER_CUT_HPP
#define SPLITTER_CUT_HPP

#include <osmium/handler/progress.hpp>

class ExtractInfo {

public:
    std::string name;
    double minlon, minlat, maxlon, maxlat;
    Osmium::Output::OSM::Base *writer;

    // the initial size of the id-trackers could be 0, because the vectors
    // are flexible, but providing here an estimation of the max. number of nodes
    // and ways gives the tool a "fail first" behaviour in the case of not enough memory 
    // (better fail in init phase, not after 5 hours of processing)
    static const unsigned int est_max_node_id =   1300000000;
    static const unsigned int est_max_way_id =     130000000;
    static const unsigned int est_max_relation_id =  1000000;

    ExtractInfo(std::string name) {
        this->name = name;
    }

    bool contains(Osmium::OSM::Node *node) {
        if(Osmium::global.debug) fprintf(stderr, "bbox-check lat(%f < %f < %f) && lon(%f < %f < %f)\n", minlat, node->get_lat(), maxlat, minlon, node->get_lon(), maxlon);
        return minlat < node->get_lat() && node->get_lat() < maxlat && minlon < node->get_lon() && node->get_lon() < maxlon;
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
            delete extracts[i];
        }
        delete pg;
    }

public:

    bool debug;

    Cut() {
        pg = new Osmium::Handler::Progress();
    }

    void addBbox(std::string name, double minlon, double minlat, double maxlon, double maxlat) {
        fprintf(stderr, "opening writer for %s\n", name.c_str());
        Osmium::Output::OSM::Base *writer = Osmium::Output::OSM::create(name);
        if(!writer->is_history_file()) {
            fprintf(stderr, "file of type %s is not able to store history information\n", name.c_str());
            throw std::runtime_error("");
        }
        writer->write_init();
        writer->write_bounds(minlon, minlat, maxlon, maxlat);

        TExtractInfo *b = new TExtractInfo(name);

        b->minlon = minlon;
        b->minlat = minlat;
        b->maxlon = maxlon;
        b->maxlat = maxlat;

        b->writer = writer;

        extracts.push_back(b);
    }
};

#endif // SPLITTER_CUT_HPP

