#include <osmium/output/xml.hpp>

class BBoxInfo {

public:
    std::string name;
    double x1, y1, x2, y2;
    Osmium::Output::XML *writer;

    bool enabled;

    std::vector<bool> node_tracker;
    std::vector<bool> way_tracker;

    BBoxInfo(std::string name) {
        this->name = name;
        this->enabled = false;
    }
};

class Cut : public Osmium::Handler::Base {

protected:

    std::vector<BBoxInfo*> bboxes;

    ~Cut() {
        for(int i=0, l = bboxes.size(); i<l; i++) {
            delete bboxes[i]->writer;
            delete bboxes[i];
        }
    }

public:

    bool debug;

    void addBbox(std::string name, double x1, double y1, double x2, double y2) {
        fprintf(stderr, "opening writer for %s\n", name.c_str());
        Osmium::Output::XML *writer = new Osmium::Output::XML(name.c_str());
        writer->writeVisibleAttr = true; // enable visible attribute
        writer->writeBounds(x1, y1, x2, y2);

        fprintf(stderr, "allocating bit-tracker\n");
        BBoxInfo *b = new BBoxInfo(name);

        b->x1 = x1;
        b->y1 = y1;
        b->x2 = x2;
        b->y2 = y2;

        b->writer = writer;

        // the initial size of the id-trackers could be 0, because the vectors
        // are flexible, but providing here an estimation of the max. number of nodes
        // and ways gives the tool a "fail first" behaviour in the case of not enough memory 
        // (better fail in init phase, not after 5 hours of processing)

        b->node_tracker = std::vector<bool>(1100000000);
        b->way_tracker = std::vector<bool>(  100000000);

        bboxes.push_back(b);
    }
};

