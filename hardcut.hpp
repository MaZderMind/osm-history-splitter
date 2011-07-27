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
 - needs (theroeticvally) only ~182,4 MB RAM per extract (practically ~190 MB RAM)
   - ((1400000000÷8)+(130000000÷8))÷1024÷1024
   - 1.4 mrd nodes & 130 mio ways, one bit each, in megabytes

disadvantages:
 - relations referring to relations that come later in the file are missing this valid references
 - ways that have only one node inside the bbox are missing from the output
 - only versions of an object that are inside the bboxes are in thr extract, some versions may be missing

*/

class HardcutExtractInfo : public ExtractInfo {

public:
    std::vector<bool> node_tracker;
    std::vector<bool> way_tracker;

    HardcutExtractInfo(std::string name) : ExtractInfo(name) {
        fprintf(stderr, "allocating bit-tracker\n");
        node_tracker = std::vector<bool>(ExtractInfo::est_max_node_id);
        way_tracker = std::vector<bool>(ExtractInfo::est_max_way_id);
    }
};

class Hardcut : public Cut<HardcutExtractInfo> {

protected:

    std::vector<Osmium::OSM::Node*> current_node_vector;

    osm_object_id_t last_id;

public:

    void init(Osmium::OSM::Meta& meta) {
        fprintf(stderr, "hardcut init\n");
        for(int i = 0, l = extracts.size(); i<l; i++) {
            fprintf(stderr, "\textract[%d] %s\n", i, extracts[i]->name.c_str());
        }

        last_id = 0;

        if(debug) fprintf(stderr, "\n\n===== NODES =====\n\n");
        else pg->init(meta);
    }

    // walk over all node-versions
    void node(Osmium::OSM::Node *e) {
        if(debug) fprintf(stderr, "hardcut node %d v%d\n", e->id(), e->version());
        else pg->node(e);

        // walk over all bboxes
        for(int i = 0, l = extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = extracts[i];

            // if the node-version is in the bbox
            if(extract->contains(e)) {
                // write the node to the writer of this bbox
                if(debug) fprintf(stderr, "node %d v%d is inside bbox[%d], writing it out\n", e->id(), e->version(), i);
                extract->writer->node(e);

                // record its id in the bboxes node-id-tracker
                if((int)extract->node_tracker.size() < e->id()) {
                    fprintf(stderr, "WARNING! node_tracker is too small to hold id %d, resizing...\n", e->id());
                    fprintf(stderr, "    TIP: increase estimation of max. node id in cut.hpp\n");
                    extract->node_tracker.reserve(e->id());
                }
                extract->node_tracker[e->id()] = true;
            }
        }

        // record the last id
        last_id = e->id();
    }

    void after_nodes() {
        if(debug) fprintf(stderr, "after nodes\n");
        else pg->after_nodes();

        last_id = 0;
        
        if(debug) fprintf(stderr, "\n\n===== WAYS =====\n\n");
    }

    // walk over all way-versions
    void way(Osmium::OSM::Way *e) {
        if(debug) fprintf(stderr, "hardcut way %d v%d\n", e->id(), e->version());
        else pg->way(e);

        // walk over all bboxes
        for(int i = 0, l = extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = extracts[i];

            // create a new way NULL pointer
            Osmium::OSM::Way *c = NULL;

            // walk over all waynodes
            for(osm_sequence_id_t ii = 0, ll = e->node_count(); ii < ll; ii++) {
                // shorthand
                osm_object_id_t node_id = e->get_node_id(ii);

                // if the waynode is in the node-id-tracker of this bbox
                if(extract->node_tracker[node_id]) {
                    // if the new way pointer is NULL
                    if(!c) {
                        // create a new way with all meta-data and tags but without waynodes
                        if(debug) fprintf(stderr, "creating cutted way %d v%d for bbox[%d]\n", e->id(), e->version(), i);
                        c = new Osmium::OSM::Way();
                        c->id(e->id());
                        c->version(e->version());
                        c->uid(e->uid());
                        c->changeset(e->changeset());
                        c->timestamp(e->timestamp());
                        c->visible(e->visible());
                        c->user(e->user());
                        for(Osmium::OSM::TagList::const_iterator it = e->tags().begin(); it != e->tags().end(); ++it) {
                            c->tags().add(it->key(), it->value());
                        }
                    }

                    // add the waynode to the new way
                    if(debug) fprintf(stderr, "adding node-id %d to cutted way %d v%d for bbox[%d]\n", node_id, e->id(), e->version(), i);
                    c->add_node(node_id);
                }
            }

            // if the way pointer is not NULL
            if(c) {
                // enable way-writing for this bbox
                if(debug) fprintf(stderr, "way %d v%d is in bbox[%d]\n", e->id(), e->version(), i);

                // check for short ways
                if(c->node_count() < 2) {
                    if(debug) fprintf(stderr, "way %d v%d in bbox[%d] would only be %d nodes long, skipping\n", e->id(), e->version(), i, c->node_count());
                    delete c;
                    c = NULL;
                    continue;
                }

                // write the way to the writer of this bbox
                if(debug) fprintf(stderr, "way %d v%d is inside bbox[%d], writing it out\n", e->id(), e->version(), i);
                extract->writer->way(c);
                delete c;
                c = NULL;

                // record its id in the bboxes way-id-tracker
                if((int)extract->way_tracker.size() < e->id()) {
                    fprintf(stderr, "WARNING! way_tracker is too small to hold id %d, resizing...\n", e->id());
                    fprintf(stderr, "    TIP: increase estimation of max. way id in cut.hpp\n");
                    extract->node_tracker.reserve(e->id());
                }
                extract->way_tracker[e->id()] = true;
            }
        }

        // record the last id
        last_id = e->id();
    }

    void after_ways() {
        if(debug) fprintf(stderr, "after ways\n");
        else pg->after_ways();

        last_id = 0;

        if(debug) fprintf(stderr, "\n\n===== RELATIONS =====\n\n");
    }

    // walk over all relation-versions
    void relation(Osmium::OSM::Relation *e) {
        if(debug) fprintf(stderr, "hardcut relation %d v%d\n", e->id(), e->version());
        else pg->relation(e);

        // walk over all bboxes
        for(int i = 0, l = extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = extracts[i];

            // create a new relation NULL pointer
            Osmium::OSM::Relation *c = NULL;

            // walk over all relation members
            for(Osmium::OSM::RelationMemberList::const_iterator it = e->members().begin(); it != e->members().end(); ++it) {
                // if the relation members is in the node-id-tracker or the way-id-tracker of this bbox
                if((it->type() == 'n' && extract->node_tracker[it->ref()]) || (it->type() == 'w' && extract->way_tracker[it->ref()])) {
                    // if the new way pointer is NULL
                    if(!c) {
                        // create a new relation with all meta-data and tags but without waynodes
                        if(debug) fprintf(stderr, "creating cutted relation %d v%d for bbox[%d]\n", e->id(), e->version(), i);
                        c = new Osmium::OSM::Relation();
                        c->id(e->id());
                        c->version(e->version());
                        c->uid(e->uid());
                        c->changeset(e->changeset());
                        c->timestamp(e->timestamp());
                        c->visible(e->visible());
                        c->user(e->user());
                        for(Osmium::OSM::TagList::const_iterator it = e->tags().begin(); it != e->tags().end(); ++it) {
                            c->tags().add(it->key(), it->value());
                        }
                    }

                    // add the member to the new relation
                    if(debug) fprintf(stderr, "adding member %c id %d to cutted relation %d v%d for bbox[%d]\n", it->type(), it->ref(), e->id(), e->version(), i);
                    c->add_member(it->type(), it->ref(), it->role());
                }
            }

            // if the relation pointer is not NULL
            if(c) {
                // write the way to the writer of this bbox
                if(debug) fprintf(stderr, "relation %d v%d is inside bbox[%d], writing it out\n", e->id(), e->version(), i);
                extract->writer->relation(c);
                delete c;
                c = NULL;
            }
        }

        // record the last id
        last_id = e->id();
    }

    void after_relations() {
        if(debug) fprintf(stderr, "after relation\n");
        else pg->after_relations();

        last_id = 0;
    }

    void final() {
        if(!debug) pg->final();
        fprintf(stderr, "hardcut finished\n");
    }
};

#endif // SPLITTER_HARDCUT_HPP

