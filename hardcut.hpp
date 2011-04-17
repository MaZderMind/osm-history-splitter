#include "cut.hpp"

class Hardcut : public Cut {
    
public:
    
    void callback_init() {
        printf("hardcut init\n");
    }
    
    void callback_node(Osmium::OSM::Node *node) {
        for(int i, l = bboxes.size(); i<l; i++) {
            printf("\tbbox %s\n", bboxes[i].name.c_str());
        }
        
        printf("hardcut node %d v%d, visi: %d\n", node->id, node->version, node->visible);
    }
    
    void callback_way(Osmium::OSM::Way *way) {
        printf("hardcut way\n");
    }

    void callback_relation(Osmium::OSM::Relation *relation) {
        printf("hardcut relation\n");
    }

    void callback_final() {
        printf("hardcut final\n");
    }
};

