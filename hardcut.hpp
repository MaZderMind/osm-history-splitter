#include "cut.hpp"

/*

Hardcut Algorithm
 - walk over all node-versions
   - if the current node-version has a new node-id
     - walk over all bboxes
       - if node-writing for this bbox is enabled
         - write all nodes from the current-node-vector to this bboxes writer
      - disable node-writing for this bbox
      - clear the current-node-vector
   
   - add the node-version to the current-node-vector
   
   - walk over all bboxes
     - if node-writing for this bbox is still disabled
       - if the node-version is in the bbox
         - enable node-writing for this bbox
         - record its id in the bboxes node-id-tracker
 
 
 
 - walk over all way-versions
   - if the current way-version has a new way-id
     - walk over all bboxes
       - if way-writing for this bbox is enabled
         - write all ways from the current-ways-vector to this bboxes writer
      - disable way-writing for this bbox
      - clear the current-way-vector
   
   - add the way-version to the current-way-vector
   
   - walk over all bboxes
     - create a new way NULL pointer
     - walk over all waynodes
       - if the waynode is in the node-id-tracker of this bbox
         - if the new way pointer is NULL
           - create a new way with all meta-data and tags but without waynodes
         - add the waynode to the new way
     
     - if the way pointer is not NULL
       - enable way-writing for this bbox
       - add the way to the current-ways-vector
       - record its id in the bboxes way-id-tracker
 
 
 
 - walk over all relation-versions
   - if the current relation-version has a new way-id
     - walk over all bboxes
       - if relation-writing for this bbox is enabled
         - write all relations from the current-relations-vector to this bboxes writer
      - disable relation-writing for this bbox
      - clear the current-relation-vector
   
   - add the relation-version to the current-relation-vector
   
   - walk over all bboxes
     - create a new relation NULL pointer
     - walk over all relation members
       - if the relation members is in the node-id-tracker or the way-id-tracker of this bbox
         - if the new relation pointer is NULL
           - create a new relation with all meta-data and tags but without members
         - add the member to the new relation
     
     - if the relation pointer is not NULL
       - enable relation-writing for this bbox
       - add the relation to the current-relations-vector

advantages:
 - single pass
 - all versions of elements are reserved
 - ways and relations are cropped at bbox boundaries

Disadvantages
 - Relations referring to Relations that come later in the file are missing this references

*/

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

