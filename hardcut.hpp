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
        std::cerr << "hardcut init" << std::endl;
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            std::cerr << "\textract[" << i << "] " << info->extracts[i]->name << std::endl;
        }

        last_id = 0;

        if(debug) std::cerr << std::endl << std::endl << "===== NODES =====" << std::endl << std::endl;
        else pg.init(meta);
    }

    // walk over all node-versions
    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        if(debug) std::cerr << "hardcut node " << node->id() << " v" << node->version() << std::endl;
        else pg.node(node);

        // walk over all bboxes
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = info->extracts[i];

            // if the node-version is in the bbox
            if(extract->contains(node)) {
                // write the node to the writer of this bbox
                if(debug) std::cerr << "node " << node->id() << " v" << node->version() << " is inside bbox[" << i << "], writing it out" << std::endl;
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
            std::cerr << "after nodes" << std::endl <<
                std::endl << std::endl << "===== WAYS =====" << std::endl << std::endl;
        } else {
            pg.after_nodes();
        }

        last_id = 0;
    }

    // walk over all way-versions
    void way(const shared_ptr<Osmium::OSM::Way const>& way) {
        if(debug) std::cerr << "hardcut way " << way->id() << " v" << way->version() << std::endl;
        else pg.way(way);

        // walk over all bboxes
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            // shorthand
            HardcutExtractInfo *extract = info->extracts[i];

            // create a new way NULL pointer
            shared_ptr<Osmium::OSM::Way> newway;

            // walk over all waynodes
            for(osm_sequence_id_t ii = 0, ll = way->nodes().size(); ii < ll; ii++) {
                // shorthand
                osm_object_id_t node_id = way->get_node_id(ii);

                // if the waynode is in the node-id-tracker of this bbox
                if(extract->node_tracker.get(node_id)) {
                    // if the new way pointer is NULL
                    if(!newway) {
                        // create a new way with all meta-data and tags but without waynodes
                        if(debug) std::cerr << "creating cutted way " << way->id() << " v" << way->version() << " for bbox[" << i << "]" << std::endl;
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
                    if(debug) std::cerr << "adding node-id " << node_id << " to cutted way " << way->id() << " v" << way->version() << " for bbox[" << i << "]" << std::endl;
                    newway->add_node(node_id);
                }
            }

            // if the way pointer is not NULL
            if(newway.get()) {
                // enable way-writing for this bbox
                if(debug) std::cerr << "way " << way->id() << " v" << way->version() << " is in bbox[" << i << "]" << std::endl;

                // check for short ways
                if(newway->nodes().size() < 2) {
                    if(debug) std::cerr << "way " << way->id() << " v" << way->version() << " in bbox[" << i << "] would only be " << newway->nodes().size() << " nodes long, skipping" << std::endl;
                    continue;
                }

                // write the way to the writer of this bbox
                if(debug) std::cerr << "way " << way->id() << " v" << way->version() << " is inside bbox[" << i << "], writing it out" << std::endl;
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
            std::cerr << "after ways" << std::endl <<
                std::endl << std::endl << "===== RELATIONS =====" << std::endl << std::endl;
        } else {
            pg.after_ways();
        }

        last_id = 0;
    }

    // walk over all relation-versions
    void relation(const shared_ptr<Osmium::OSM::Relation const>& relation) {
        if(debug) std::cerr << "hardcut relation " << relation->id() << " v" << relation->version() << std::endl;
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
                        if(debug) std::cerr << "creating cutted relation " << relation->id() << " v" << relation->version() << " for bbox[" << i << "]" << std::endl;
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
                    if(debug) std::cerr << "adding member " << it->type() << " id " << it->ref() << " to cutted relation " << relation->id() << " v" << relation->version() << "for bbox[" << i << "]" << std::endl;
                    newrelation->add_member(it->type(), it->ref(), it->role());
                }
            }

            // if the relation pointer is not NULL
            if(newrelation.get()) {
                // write the way to the writer of this bbox
                if(debug) std::cerr << "relation " << relation->id() << " v" << relation->version() << " is inside bbox[" << i << "], writing it out" << std::endl;
                extract->writer->relation(newrelation);
            }
        }

        // record the last id
        last_id = relation->id();
    }

    void after_relations() {
        if(debug) {
            std::cerr << "after relation" << std::endl;
        } else {
            pg.after_relations();
        }

        last_id = 0;
    }

    void final() {
        if(!debug) {
            pg.final();
        }

        std::cerr << "hardcut finished" << std::endl;
    }
};

#endif // SPLITTER_HARDCUT_HPP

