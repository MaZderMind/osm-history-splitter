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
    growing_bitset node_tracker;
    growing_bitset way_tracker;

    HardcutExtractInfo(std::string name) : ExtractInfo(name) {}
};

class HardcutInfo : public CutInfo<HardcutExtractInfo> {

};

class Hardcut : public Cut<HardcutInfo> {

protected:

    std::vector<Osmium::OSM::Node*> current_node_vector;

    osm_object_id_t last_id;

public:

    Hardcut(HardcutInfo *info) : Cut<HardcutInfo>(info) {}

    void init(Osmium::OSM::Meta& meta) {
        fprintf(stderr, "hardcut init\n");
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            fprintf(stderr, "\textract[%d] %s\n", i, info->extracts[i]->name.c_str());
        }

        last_id = 0;

        if(debug) fprintf(stderr, "\n\n===== NODES =====\n\n");
        else pg.init(meta);
    }

    // walk over all node-versions
    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        if(debug) fprintf(stderr, "hardcut node %d v%d\n", node->id(), node->version());
        else pg.node(node);

        // walk over all bboxes
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = info->extracts[i];

            // if the node-version is in the bbox
            if(extract->contains(node)) {
                // write the node to the writer of this bbox
                if(debug) fprintf(stderr, "node %d v%d is inside bbox[%d], writing it out\n", node->id(), node->version(), i);
                extract->writer->node(node);

                // record its id in the bboxes node-id-tracker
                extract->node_tracker.set(node->id());
            }
        }

        // record the last id
        last_id = node->id();
    }

    void after_nodes() {
        if(debug) {
            fprintf(stderr, "after nodes\n");
            fprintf(stderr, "\n\n===== WAYS =====\n\n");
        } else {
            pg.after_nodes();
        }

        last_id = 0;
    }

    // walk over all way-versions
    void way(const shared_ptr<Osmium::OSM::Way const>& way) {
        if(debug) fprintf(stderr, "hardcut way %d v%d\n", way->id(), way->version());
        else pg.way(way);

        // walk over all bboxes
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = info->extracts[i];

            // create a new way NULL pointer
            shared_ptr<Osmium::OSM::Way> newway;

            // walk over all waynodes
            for(osm_sequence_id_t ii = 0, ll = way->node_count(); ii < ll; ii++) {
                // shorthand
                osm_object_id_t node_id = way->get_node_id(ii);

                // if the waynode is in the node-id-tracker of this bbox
                if(extract->node_tracker.get(node_id)) {
                    // if the new way pointer is NULL
                    if(!newway) {
                        // create a new way with all meta-data and tags but without waynodes
                        if(debug) fprintf(stderr, "creating cutted way %d v%d for bbox[%d]\n", way->id(), way->version(), i);
                        newway = shared_ptr<Osmium::OSM::Way>(new Osmium::OSM::Way());
                        newway->id(way->id());
                        newway->version(way->version());
                        newway->uid(way->uid());
                        newway->changeset(way->changeset());
                        newway->timestamp(way->timestamp());
                        newway->visible(way->visible());
                        newway->user(way->user());
                        for(Osmium::OSM::TagList::const_iterator it = way->tags().begin(); it != way->tags().end(); ++it) {
                            newway->tags().add(it->key(), it->value());
                        }
                    }

                    // add the waynode to the new way
                    if(debug) fprintf(stderr, "adding node-id %d to cutted way %d v%d for bbox[%d]\n", node_id, way->id(), way->version(), i);
                    newway->add_node(node_id);
                }
            }

            // if the way pointer is not NULL
            if(newway.get()) {
                // enable way-writing for this bbox
                if(debug) fprintf(stderr, "way %d v%d is in bbox[%d]\n", way->id(), way->version(), i);

                // check for short ways
                if(newway->node_count() < 2) {
                    if(debug) fprintf(stderr, "way %d v%d in bbox[%d] would only be %d nodes long, skipping\n", way->id(), way->version(), i, newway->node_count());
                    continue;
                }

                // write the way to the writer of this bbox
                if(debug) fprintf(stderr, "way %d v%d is inside bbox[%d], writing it out\n", way->id(), way->version(), i);
                extract->writer->way(newway);

                // record its id in the bboxes way-id-tracker
                extract->way_tracker.set(way->id());
            }
        }

        // record the last id
        last_id = way->id();
    }

    void after_ways() {
        if(debug) {
            fprintf(stderr, "after ways\n");
            fprintf(stderr, "\n\n===== RELATIONS =====\n\n");
        } else {
            pg.after_ways();
        }

        last_id = 0;
    }

    // walk over all relation-versions
    void relation(const shared_ptr<Osmium::OSM::Relation const>& relation) {
        if(debug) fprintf(stderr, "hardcut relation %d v%d\n", relation->id(), relation->version());
        else pg.relation(relation);

        // walk over all bboxes
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = info->extracts[i];

            // create a new relation NULL pointer
            shared_ptr<Osmium::OSM::Relation> newrelation;

            // walk over all relation members
            for(Osmium::OSM::RelationMemberList::const_iterator it = relation->members().begin(); it != relation->members().end(); ++it) {
                // if the relation members is in the node-id-tracker or the way-id-tracker of this bbox
                if((it->type() == 'n' && extract->node_tracker.get(it->ref())) || (it->type() == 'w' && extract->way_tracker.get(it->ref()))) {
                    // if the new way pointer is NULL
                    if(!newrelation) {
                        // create a new relation with all meta-data and tags but without waynodes
                        if(debug) fprintf(stderr, "creating cutted relation %d v%d for bbox[%d]\n", relation->id(), relation->version(), i);
                        newrelation = shared_ptr<Osmium::OSM::Relation>(new Osmium::OSM::Relation());
                        newrelation->id(relation->id());
                        newrelation->version(relation->version());
                        newrelation->uid(relation->uid());
                        newrelation->changeset(relation->changeset());
                        newrelation->timestamp(relation->timestamp());
                        newrelation->visible(relation->visible());
                        newrelation->user(relation->user());
                        for(Osmium::OSM::TagList::const_iterator it = relation->tags().begin(); it != relation->tags().end(); ++it) {
                            newrelation->tags().add(it->key(), it->value());
                        }
                    }

                    // add the member to the new relation
                    if(debug) fprintf(stderr, "adding member %c id %d to cutted relation %d v%d for bbox[%d]\n", it->type(), it->ref(), relation->id(), relation->version(), i);
                    newrelation->add_member(it->type(), it->ref(), it->role());
                }
            }

            // if the relation pointer is not NULL
            if(newrelation.get()) {
                // write the way to the writer of this bbox
                if(debug) fprintf(stderr, "relation %d v%d is inside bbox[%d], writing it out\n", relation->id(), relation->version(), i);
                extract->writer->relation(newrelation);
            }
        }

        // record the last id
        last_id = relation->id();
    }

    void after_relations() {
        if(debug) {
            fprintf(stderr, "after relation\n");
        } else {
            pg.after_relations();
        }

        last_id = 0;
    }

    void final() {
        if(!debug) {
            pg.final();
        }

        fprintf(stderr, "hardcut finished\n");
    }
};

#endif // SPLITTER_HARDCUT_HPP

