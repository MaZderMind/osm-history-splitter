#include "cut.hpp"

class Hardcut : public Cut {
    
public:
    
    void callback_init() {
        printf("hardcut init\n");
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            printf("\tbbox %s\n", bboxes[i].name.c_str());
        }
    }
    
    void callback_node(Osmium::OSM::Node *e) {
        printf("hardcut node %d v%d, visi: %d\n", e->id, e->version, e->visible);
        for(int i = 0, l = bboxes.size(); i<l; i++)
            bboxes[i].writer->write(e);
    }
    
    void callback_way(Osmium::OSM::Way *e) {
        printf("hardcut way\n");
        for(int i = 0, l = bboxes.size(); i<l; i++)
            bboxes[i].writer->write(e);
    }

    void callback_relation(Osmium::OSM::Relation *e) {
        printf("hardcut relation\n");
        for(int i = 0, l = bboxes.size(); i<l; i++)
            bboxes[i].writer->write(e);
    }

    void callback_final() {
        printf("hardcut final\n");
    }
};

