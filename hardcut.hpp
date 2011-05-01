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
         - write all ways from the current-way-vector to this bboxes writer
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
       - add the way to the current-way-vector
       - record its id in the bboxes way-id-tracker
 
 
 
 - walk over all relation-versions
   - if the current relation-version has a new way-id
     - walk over all bboxes
       - if relation-writing for this bbox is enabled
         - write all relations from the current-relation-vector to this bboxes writer
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
    
protected:
    
    std::vector<Osmium::OSM::Node*> current_node_vector;
    std::vector<Osmium::OSM::Way*> current_way_vector;
    std::vector<Osmium::OSM::Relation*> current_relation_vector;
    
    osm_object_id_t last_id;
    osm_object_type_t last_type;
    
public:
    
    void callback_init() {
        printf("hardcut init\n");
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            printf("\tbbox[%d] %s\n", i, bboxes[i].name.c_str());
        }
        
        last_id = 0;
    }
    
    // walk over all node-versions
    void callback_node(Osmium::OSM::Node *e) {
        if(debug) printf("hardcut node %d v%d, visi: %d\n", e->id, e->version, e->visible);
        
        // if the current node-version has a new node-id
        if(last_id > 0 && last_id != e->id) {
            
            if(debug) fprintf(stderr, "new node-id %d (last one was %d)\n", e->id, last_id);
            callback_after_nodes();
        }
        
        // add the node-version to the current-node-vector
        if(debug) fprintf(stderr, "pushing node %d v%d into current_node_vector\n", e->id, e->version);
        Osmium::OSM::Node *clone = new Osmium::OSM::Node(*e);
        current_node_vector.push_back(clone);
        
        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            
            // if node-writing for this bbox is still disabled
            if(!bboxes[i].enabled) {
                
                // if the node-version is in the bbox
                if(debug) fprintf(stderr, "bbox[%d]-check lat(%f < %f < %f) && lon(%f < %f < %f)\n", i, bboxes[i].x1, e->get_lat(), bboxes[i].x2, bboxes[i].y1, e->get_lon(), bboxes[i].y2);
                if(e->get_lat() > bboxes[i].x1 && e->get_lat() < bboxes[i].x2 && e->get_lon() > bboxes[i].y1 && e->get_lon() < bboxes[i].y2) {
                    
                    if(debug) fprintf(stderr, "node %d v%d is inside bbox[%d], enabling node-writing for bbox[%d]\n", e->id, e->version, i, i);
                    
                    // enable node-writing for this bbox
                    bboxes[i].enabled = true;
                    
                    // record its id in the bboxes node-id-tracker
                    // TODO
                }
            }
        }
        
        // record the last id and type
        last_id = e->id;
        last_type = NODE;
    }
    
    void callback_after_nodes() {
        if(debug) fprintf(stderr, "doing post-node processing\n");
        
        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            
            // if node-writing for this bbox is enabled
            if(bboxes[i].enabled) {
                
                if(debug) fprintf(stderr, "node-writing is enabled for bbox[%d]\n", i);
                
                // write all nodes from the current-node-vector to this bboxes writer
                for(int ii = 0, ll = current_node_vector.size(); ii < ll; ii++) {
                    
                    Osmium::OSM::Node *cur = current_node_vector[ii];
                    
                    if(debug) fprintf(stderr, "writing node %d v%d (index %d in current_node_vector) to writer of bbox[%d]\n", cur->id, cur->version, ii, i);
                    
                    // bboxes writer
                    bboxes[i].writer->write(cur);
                }
                
                // disable node-writing for this bbox
                bboxes[i].enabled = false;
            }
         }
         
         // clear the current-node-vector
         if(debug) fprintf(stderr, "clearing current_node_vector\n");
         for(int ii = 0, ll = current_node_vector.size(); ii < ll; ii++) {
            delete current_node_vector[ii];
         }
         current_node_vector.clear();
    }
    
    void callback_way(Osmium::OSM::Way *e) {
        //printf("hardcut way\n");
        //for(int i = 0, l = bboxes.size(); i<l; i++)
        //    bboxes[i].writer->write(e);
    }

    void callback_relation(Osmium::OSM::Relation *e) {
        //printf("hardcut relation\n");
        //for(int i = 0, l = bboxes.size(); i<l; i++)
        //    bboxes[i].writer->write(e);
    }

    void callback_final() {
        printf("hardcut finished\n");
    }
};

