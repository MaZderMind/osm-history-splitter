#ifndef SPLITTER_HARDCUT_HPP
#define SPLITTER_HARDCUT_HPP

#include "cut.hpp"

/*

Hardcut Algorithm
 - walk over all node-versions
   - walk over all bboxes
     - if node-writing for this bbox is still disabled
       - if the node-version is in the bbox
         - write the node to this bboxes writer
         - record its id in the bboxes node-id-tracker



 - walk over all way-versions
   - walk over all bboxes
     - create a new way NULL pointer
     - walk over all waynodes
       - if the waynode is in the node-id-tracker of this bbox
         - if the new way pointer is NULL
           - create a new way with all meta-data and tags but without waynodes
         - add the waynode to the new way

     - if the way pointer is not NULL
       - if the way has <2 waynodes
         - continue with the next way-version
       - write the way to this bboxes writer
       - record its id in the bboxes way-id-tracker



 - walk over all relation-versions
   - walk over all bboxes
     - create a new relation NULL pointer
     - walk over all relation members
       - if the relation member is in the node-id-tracker or the way-id-tracker of this bbox
         - if the new relation pointer is NULL
           - create a new relation with all meta-data and tags but without members
         - add the member to the new relation

     - if the relation pointer is not NULL
       - write the relation to this bboxes writer

features:
 - single pass
 - ways are cropped at bbox boundaries
 - relations contain only members that exist in the file
 - ways and relations are reference-complete
 - needs only ~170 MB RAM per BBOX

disadvantages:
 - relations referring to relations that come later in the file are missing this valid references
 - ways that have only one node inside the bbox are missing from the output
 - only versions of an object that are inside the bboxes are in thr extract, some versions may be missing

*/

class HardcutBBoxInfo : public BBoxInfo {

public:
    std::vector<bool> node_tracker;
    std::vector<bool> way_tracker;

    HardcutBBoxInfo(std::string name) : BBoxInfo(name) {
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

        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // if the node-version is in the bbox
            if(debug) fprintf(stderr, "bbox[%d]-check lat(%f < %f < %f) && lon(%f < %f < %f)\n", i, bbox->minlat, e->get_lat(), bbox->maxlat, bbox->minlon, e->get_lon(), bbox->maxlon);
            if(bbox->minlat < e->get_lat() && e->get_lat() < bbox->maxlat && bbox->minlon < e->get_lon() && e->get_lon() < bbox->maxlon) {

                // write the node to the writer of this bbox
                if(debug) fprintf(stderr, "node %d v%d is inside bbox[%d], writing it out\n", e->id, e->version, i);
                bbox->writer->write(e);

                // record its id in the bboxes node-id-tracker
                if((int)bbox->node_tracker.size() < e->id) {
                    fprintf(stderr, "WARNING! node_tracker is too small to hold id %d, resizing...\n", e->id);
                    fprintf(stderr, "    TIP: increase estimation of max. node id in cut.hpp\n");
                    bbox->node_tracker.reserve(e->id);
                }
                bbox->node_tracker[e->id] = true;
            }
        }

        // record the last id
        last_id = e->id;
    }

    void callback_after_nodes() {
        if(debug) fprintf(stderr, "after nodes\n");
        else pg->callback_after_nodes();

        last_id = 0;
        
        if(debug) fprintf(stderr, "\n\n===== WAYS =====\n\n");
    }

    // walk over all way-versions
    void callback_way(Osmium::OSM::Way *e) {
        if(debug) fprintf(stderr, "hardcut way %d v%d\n", e->id, e->version);
        else pg->callback_way(e);

        // walk over all bboxes
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            // shorthand
            HardcutBBoxInfo *bbox = bboxes[i];

            // create a new way NULL pointer
            Osmium::OSM::Way *c = NULL;

            // walk over all waynodes
            for(int ii = 0, ll = e->node_count(); ii < ll; ii++) {
                // shorthand
                osm_object_id_t node_id = e->get_node_id(ii);

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
                    c->add_node(node_id);
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

                // write the way to the writer of this bbox
                if(debug) fprintf(stderr, "way %d v%d is inside bbox[%d], writing it out\n", e->id, e->version, i);
                bbox->writer->write(e);

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

    void callback_after_ways() {
        if(debug) fprintf(stderr, "after ways\n");
        else pg->callback_after_ways();

        last_id = 0;

        if(debug) fprintf(stderr, "\n\n===== RELATIONS =====\n\n");
    }

    // walk over all relation-versions
    void callback_relation(Osmium::OSM::Relation *e) {
        if(debug) fprintf(stderr, "hardcut relation %d v%d\n", e->id, e->version);
        else pg->callback_relation(e);

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
                    c->add_member(member->get_type(), member->get_ref(), member->get_role());
                }
            }

            // if the relation pointer is not NULL
            if(c) {
                // write the way to the writer of this bbox
                if(debug) fprintf(stderr, "relation %d v%d is inside bbox[%d], writing it out\n", e->id, e->version, i);
                bbox->writer->write(e);
            }
        }

        // record the last id
        last_id = e->id;
    }

    void callback_after_relations() {
        if(debug) fprintf(stderr, "after relation\n");
        else pg->callback_after_relations();

        last_id = 0;
    }

    void callback_final() {
        if(!debug) pg->callback_final();
        fprintf(stderr, "hardcut finished\n");
    }
};

#endif // SPLITTER_HARDCUT_HPP

