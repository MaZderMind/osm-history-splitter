#ifndef SPLITTER_HARDCUT_HPP
#define SPLITTER_HARDCUT_HPP

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

   - walk over all bboxes
     - create a new way NULL pointer
     - walk over all waynodes
       - if the waynode is in the node-id-tracker of this bbox
         - if the new way pointer is NULL
           - create a new way with all meta-data and tags but without waynodes
         - add the waynode to the new way

     - if the way pointer is not NULL
       - if the way has <2 waynodes
         - delete it and continue with the next way-version
       - enable way-writing for this bbox
       - add the way to the current-way-vector
       - record its id in the bboxes way-id-tracker



 - walk over all relation-versions
   - if the current relation-version has a new relation-id
     - walk over all bboxes
       - if relation-writing for this bbox is enabled
         - write all relations from the current-relation-vector to this bboxes writer
      - disable relation-writing for this bbox
      - clear the current-relation-vector

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

features:
 - single pass
 - if an object is in the extract, all versions of it are there
 - ways are cropped at bbox boundaries
 - relations contain only members that exist in the file
 - ways and relations are reference-complete
 - needs only 170 MB RAM per BBOX

disadvantages
 - relations referring to relations that come later in the file are missing this valid references
 - ways that have only one node inside the bbox are missing from the output

*/

class HardcutBBoxInfo : public BBoxInfo {

public:
    bool enabled;

    std::vector<Osmium::OSM::Way*> way_vector;
    std::vector<Osmium::OSM::Relation*> relation_vector;

    std::vector<bool> node_tracker;
    std::vector<bool> way_tracker;

    HardcutBBoxInfo(std::string name) : BBoxInfo(name) {
        enabled = false;

        fprintf(stderr, "allocating bit-tracker\n");
        node_tracker = std::vector<bool>(BBoxInfo::est_max_node_id);
        way_tracker = std::vector<bool>(BBoxInfo::est_max_way_id);
    }
};

class Hardcut : public Cut<HardcutBBoxInfo> {

protected:

    std::vector<Osmium::OSM::Node*> current_node_vector;

    osm_object_id_t last_id;

public:

    void callback_init() {
        fprintf(stderr, "hardcut init\n");
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            fprintf(stderr, "\tbbox[%d] %s\n", i, bboxes[i]->name.c_str());
        }

        last_id = 0;

        if(debug) fprintf(stderr, "\n\n===== NODES =====\n\n");
        else pg->callback_init();
    }

    // walk over all node-versions
    void callback_node(Osmium::OSM::Node *e) {
        if(debug) fprintf(stderr, "hardcut node %d v%d\n", e->id, e->version);
        else pg->callback_node(e);

        // if the current node-version has a new node-id
        if(last_id > 0 && last_id != e->id) {
            // post-process this node
            if(debug) fprintf(stderr, "new node-id %d (last one was %d)\n", e->id, last_id);
            post_node_proc();
        }

        // add the node-version to the current-node-vector
        if(debug) fprintf(stderr, "pushing node %d v%d into current_node_vector\n", e->id, e->version);
        Osmium::OSM::Node *clone = new Osmium::OSM::Node(*e);
        current_node_vector.push_back(clone);

        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // if node-writing for this bbox is still disabled
            if(!bbox->enabled) {

                // if the node-version is in the bbox
                if(debug) fprintf(stderr, "bbox[%d]-check lat(%f < %f < %f) && lon(%f < %f < %f)\n", i, bbox->x1, e->get_lat(), bbox->x2, bbox->y1, e->get_lon(), bbox->y2);
                if(e->get_lat() > bboxes[i]->x1 && e->get_lat() < bbox->x2 && e->get_lon() > bbox->y1 && e->get_lon() < bbox->y2) {

                    if(debug) fprintf(stderr, "node %d v%d is inside bbox[%d], enabling node-writing for bbox[%d]\n", e->id, e->version, i, i);

                    // enable node-writing for this bbox
                    bbox->enabled = true;

                    // record its id in the bboxes node-id-tracker
                    if((int)bbox->node_tracker.size() < e->id) {
                        fprintf(stderr, "WARNING! node_tracker is too small to hold id %d, resizing...\n", e->id);
                        fprintf(stderr, "    TIP: increase estimation of max. node id in cut.hpp\n");
                        bbox->node_tracker.reserve(e->id);
                    }
                    bbox->node_tracker[e->id] = true;
                }
            }
        }

        // record the last id
        last_id = e->id;
    }
    
    void post_node_proc() {
        if(debug) fprintf(stderr, "doing post-node processing\n");
        
        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // if node-writing for this bbox is enabled
            if(bbox->enabled) {

                if(debug) fprintf(stderr, "node-writing is enabled for bbox[%d]\n", i);

                // write all nodes from the current-node-vector to this bboxes writer
                for(int ii = 0, ll = current_node_vector.size(); ii < ll; ii++) {

                    Osmium::OSM::Node *cur = current_node_vector[ii];

                    if(debug) fprintf(stderr, "writing node %d v%d (index %d in current_node_vector) to writer of bbox[%d]\n", cur->id, cur->version, ii, i);

                    // bboxes writer
                    bbox->writer->write(cur);
                }

                // disable node-writing for this bbox
                bbox->enabled = false;
            }
         }

         // clear the current-node-vector
         if(debug) fprintf(stderr, "clearing current_node_vector\n");
         for(int ii = 0, ll = current_node_vector.size(); ii < ll; ii++) {
            delete current_node_vector[ii];
         }
         current_node_vector.clear();
    }

    void callback_after_nodes() {
        if(debug) fprintf(stderr, "after nodes\n");
        else pg->callback_after_nodes();

        post_node_proc();
        last_id = 0;
        
        if(debug) fprintf(stderr, "\n\n===== WAYS =====\n\n");
    }

    // walk over all way-versions
    void callback_way(Osmium::OSM::Way *e) {
        if(debug) fprintf(stderr, "hardcut way %d v%d\n", e->id, e->version);
        else pg->callback_way(e);

        // if the current way-version has a new way-id
        if(last_id > 0 && last_id != e->id) {
            // post-process this way
            if(debug) fprintf(stderr, "new way-id %d (last one was %d)\n", e->id, last_id);
            post_way_proc();
        }

        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // create a new way NULL pointer
            Osmium::OSM::Way *c = NULL;

            // walk over all waynodes
            for(int ii = 0, ll = e->node_count(); ii < ll; ii++) {
                // shorthand
                osm_object_id_t node_id = e->nodes[ii];

                // if the waynode is in the node-id-tracker of this bbox
                if(bbox->node_tracker[node_id]) {
                    // if the new way pointer is NULL
                    if(!c) {
                        // create a new way with all meta-data and tags but without waynodes
                        if(debug) fprintf(stderr, "creating cutted way %d v%d for bbox[%d]\n", e->id, e->version, i);
                        c = new Osmium::OSM::Way();
                        c->id        = e->id;
                        c->version   = e->version;
                        c->uid       = e->uid;
                        c->changeset = e->changeset;
                        c->timestamp = e->timestamp;
                        c->visible   = e->visible;
                        strncpy(c->user, e->user, Osmium::OSM::Object::max_length_username);
                        for(int ti = 0, tl = e->tag_count(); ti < tl; ti++) {
                            c->add_tag(e->get_tag_key(ti), e->get_tag_value(ti));
                        }
                    }

                    // add the waynode to the new way
                    if(debug) fprintf(stderr, "adding node-id %d to cutted way %d v%d for bbox[%d]\n", node_id, e->id, e->version, i);
                    try
                    {
                        c->add_node(node_id);
                    }
                    catch(std::range_error&)
                    {
                        fprintf(stderr, "error adding node-id %d to cutted way %d v%d: it's already full (std::range_error)\n", node_id, e->id, e->version);
                    }
                }
            }

            // if the way pointer is not NULL
            if(c) {
                // enable way-writing for this bbox
                if(debug) fprintf(stderr, "way %d v%d is in bbox[%d]\n", e->id, e->version, i);

                // check for short ways
                if(c->node_count() < 2) {
                    if(debug) fprintf(stderr, "way %d v%d in bbox[%d] would only be %d nodes long, skipping\n", e->id, e->version, i, c->node_count());
                    delete c;
                    c = NULL;
                    continue;
                }

                bbox->enabled = true;

                // add the way to the current-way-vector
                if(debug) fprintf(stderr, "pushing way %d v%d into current_way_vector of bbox[%d]\n", e->id, e->version, i);
                bbox->way_vector.push_back(c);

                // record its id in the bboxes way-id-tracker
                if((int)bbox->way_tracker.size() < e->id) {
                    fprintf(stderr, "WARNING! way_tracker is too small to hold id %d, resizing...\n", e->id);
                    fprintf(stderr, "    TIP: increase estimation of max. way id in cut.hpp\n");
                    bbox->node_tracker.reserve(e->id);
                }
                bbox->way_tracker[e->id] = true;
            }
        }

        // record the last id
        last_id = e->id;
    }

    void post_way_proc() {
        if(debug) fprintf(stderr, "doing post-way processing\n");

        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // if way-writing for this bbox is enabled
            if(bbox->enabled) {

                if(debug) fprintf(stderr, "way-writing is enabled for bbox[%d]\n", i);

                // write all ways from the current-way-vector to this bboxes writer
                for(int ii = 0, ll = bbox->way_vector.size(); ii < ll; ii++) {

                    Osmium::OSM::Way *cur = bbox->way_vector[ii];

                    if(debug) fprintf(stderr, "writing way %d v%d (index %d in current_way_vector) to writer of bbox[%d]\n", cur->id, cur->version, ii, i);

                    // bboxes writer
                    bbox->writer->write(cur);
                }

                // disable way-writing for this bbox
                bbox->enabled = false;
            }

            // clear the current-way-vector
            if(debug) fprintf(stderr, "clearing current_way_vector of bbox[%d]\n", i);
            for(int ii = 0, ll = bbox->way_vector.size(); ii < ll; ii++) {
                delete bbox->way_vector[ii];
            }
            bbox->way_vector.clear();
         }
    }

    void callback_after_ways() {
        if(debug) fprintf(stderr, "after ways\n");
        else pg->callback_after_ways();

        post_way_proc();
        last_id = 0;

        if(debug) fprintf(stderr, "\n\n===== RELATIONS =====\n\n");
    }

    // walk over all relation-versions
    void callback_relation(Osmium::OSM::Relation *e) {
        if(debug) fprintf(stderr, "hardcut relation %d v%d\n", e->id, e->version);
        else pg->callback_relation(e);

        // if the current relation-version has a new relation-id
        if(last_id > 0 && last_id != e->id) {
            // post-process this relation
            if(debug) fprintf(stderr, "new relation-id %d (last one was %d)\n", e->id, last_id);
            post_relation_proc();
        }

        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // create a new relation NULL pointer
            Osmium::OSM::Relation *c = NULL;

            // walk over all relation members
            for(int ii = 0, ll = e->member_count(); ii < ll; ii++) {
                // shorthand
                const Osmium::OSM::RelationMember *member = e->get_member(ii);

                // if the relation members is in the node-id-tracker or the way-id-tracker of this bbox
                if((member->get_type() == 'n' && bbox->node_tracker[member->get_ref()]) || (member->get_type() == 'w' && bbox->way_tracker[member->get_ref()])) {
                    // if the new way pointer is NULL
                    if(!c) {
                        // create a new relation with all meta-data and tags but without waynodes
                        if(debug) fprintf(stderr, "creating cutted relation %d v%d for bbox[%d]\n", e->id, e->version, i);
                        c = new Osmium::OSM::Relation();
                        c->id        = e->id;
                        c->version   = e->version;
                        c->uid       = e->uid;
                        c->changeset = e->changeset;
                        c->timestamp = e->timestamp;
                        c->visible   = e->visible;
                        strncpy(c->user, e->user, Osmium::OSM::Object::max_length_username);
                        for(int ti = 0, tl = e->tag_count(); ti < tl; ti++) {
                            c->add_tag(e->get_tag_key(ti), e->get_tag_value(ti));
                        }
                    }

                    // add the member to the new relation
                    if(debug) fprintf(stderr, "adding member %c id %d to cutted relation %d v%d for bbox[%d]\n", member->get_type(), member->get_ref(), e->id, e->version, i);
                    try
                    {
                        c->add_member(member->get_type(), member->get_ref(), member->get_role());
                    }
                    catch(std::range_error&)
                    {
                        fprintf(stderr, "error adding member %c id %d to cutted way %d v%d: it's already full (std::range_error)\n", member->get_type(), member->get_ref(), e->id, e->version);
                    }
                }
            }

            // if the relation pointer is not NULL
            if(c) {
                // enable relation-writing for this bbox
                if(debug) fprintf(stderr, "relation %d v%d is in bbox[%d]\n", e->id, e->version, i);

                bbox->enabled = true;

                // add the way to the current-way-vector
                if(debug) fprintf(stderr, "pushing relation %d v%d into current_relation_vector of bbox[%d]\n", e->id, e->version, i);
                bbox->relation_vector.push_back(c);
            }
        }

        // record the last id
        last_id = e->id;
    }

    void post_relation_proc() {
        if(debug) fprintf(stderr, "doing post-relation processing\n");

        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // if relation-writing for this bbox is enabled
            if(bbox->enabled) {

                if(debug) fprintf(stderr, "relation-writing is enabled for bbox[%d]\n", i);

                // write all relations from the current-relation-vector to this bboxes writer
                for(int ii = 0, ll = bbox->relation_vector.size(); ii < ll; ii++) {

                    Osmium::OSM::Relation *cur = bbox->relation_vector[ii];

                    if(debug) fprintf(stderr, "writing relation %d v%d (index %d in current_relation_vector) to writer of bbox[%d]\n", cur->id, cur->version, ii, i);

                    // bboxes writer
                    bbox->writer->write(cur);
                }

                // disable relation-writing for this bbox
                bbox->enabled = false;
            }

            // clear the current-way-vector
            if(debug) fprintf(stderr, "clearing current_relation_vector of bbox[%d]\n", i);
            for(int ii = 0, ll = bbox->relation_vector.size(); ii < ll; ii++) {
                delete bbox->relation_vector[ii];
            }
            bbox->relation_vector.clear();
         }
    }

    void callback_after_relations() {
        if(debug) fprintf(stderr, "after relation\n");
        else pg->callback_after_relations();

        post_relation_proc();
        last_id = 0;
    }

    void callback_final() {
        if(!debug) pg->callback_final();
        fprintf(stderr, "hardcut finished\n");
    }
};

#endif // SPLITTER_HARDCUT_HPP

