#ifndef SPLITTER_SOFTCUT_HPP
#define SPLITTER_SOFTCUT_HPP

#include "cut.hpp"

/*

Softcut Algorithm
 - walk over all node-versions
   - walk over all bboxes
     - if the current node-version is inside the bbox
       - record its id in the bboxes node-tracker

 - walk over all way-versions
   - walk over all bboxes
     - walk over all way-nodes
       - record the way-node-id in the current-way-node-vector
       - if the way-node is recorded in the bboxes node-tracker
         - record its id in the bboxes way-id-tracker

     - if its id is in the bboxes way-tracker
       - walk over the current-way-node-vector
         - record its id in the bboxes extra-node-tracker

 - walk over all relation-versions
   - walk over all bboxes
     - walk over all relation-members
       - if the relation-member is recorded in the bboxes node- or way-tracker
         - record its id in the bboxes relation-tracker

Second Pass
 - walk over all node-versions
   - walk over all bboxes
     - if the node-id is recorded in the bboxes node-tracker or in the extra-node-tracker
       - send the node to the bboxes writer

 - walk over all way-versions
   - walk over all bboxes
     - if the way-id is recorded in the bboxes way-tracker
       - send the way to the bboxes writer

 - walk over all relation-versions
   - walk over all bboxes
     - if the relation-id is recorded in the bboxes relation-tracker
       - send the relation to the bboxes writer

features:
 - if an object is in the extract, all versions of it are there
 - ways and relations are not changed
 - ways are reference-complete

disadvantages
 - dual pass
 - needs more RAM: 325 MB per BBOX
 - relations will have dead references

*/


class SoftcutExtractInfo : public ExtractInfo {

public:
    std::vector<bool> node_tracker;
    std::vector<bool> extra_node_tracker;
    std::vector<bool> way_tracker;
    std::vector<bool> relation_tracker;

    SoftcutExtractInfo(std::string name) : ExtractInfo(name) {
        fprintf(stderr, "allocating bit-tracker\n");
        node_tracker = std::vector<bool>(ExtractInfo::est_max_node_id);
        extra_node_tracker = std::vector<bool>(ExtractInfo::est_max_node_id);
        way_tracker = std::vector<bool>(ExtractInfo::est_max_way_id);
        relation_tracker = std::vector<bool>(ExtractInfo::est_max_relation_id);
    }
};

class SoftcutInfo : public CutInfo<SoftcutExtractInfo> {

};


class SoftcutPassOne : public Cut<SoftcutInfo> {

public:
    SoftcutPassOne(SoftcutInfo *info) : Cut<SoftcutInfo>(info) {}

    void init(Osmium::OSM::Meta& meta) {
        fprintf(stderr, "softcut first-pass init\n");
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            fprintf(stderr, "\textract[%d] %s\n", i, info->extracts[i]->name.c_str());
        }

        if(debug) {
            fprintf(stderr, "\n\n===== NODES =====\n\n");
        } else {
            pg.init(meta);
        }
    }


    void node(Osmium::OSM::Node *node) {
        if(debug) {
            fprintf(stderr, "softcut node %d v%d\n", node->id(), node->version());
        } else {
            pg.node(node);
        }
    }

    void after_nodes() {
        if(debug) {
            fprintf(stderr, "after nodes\n");
            fprintf(stderr, "\n\n===== WAYS =====\n\n");
        } else {
            pg.after_nodes();
        }
    }


    void way(Osmium::OSM::Way *way) {
        if(debug) {
            fprintf(stderr, "softcut way %d v%d\n", way->id(), way->version());
        } else {
            pg.way(way);
        }
    }

    void after_ways() {
        if(debug) {
            fprintf(stderr, "after ways\n");
            fprintf(stderr, "\n\n===== RELATIONS =====\n\n");
        }
        else {
            pg.after_ways();
        }
    }


    void relation(Osmium::OSM::Relation *relation) {
        if(debug) {
            fprintf(stderr, "softcut relation %d v%d\n", relation->id(), relation->version());
        } else {
            pg.relation(relation);
        }
    }

    void after_relations() {
        if(debug) {
            fprintf(stderr, "after relations\n");
        } else {
            pg.after_relations();
        }
    }

    void final() {
        fprintf(stderr, "softcut first-pass finished\n");
        if(!debug) {
            pg.final();
        }
    }
};





class SoftcutPassTwo : public Cut<SoftcutInfo> {

public:
    SoftcutPassTwo(SoftcutInfo *info) : Cut<SoftcutInfo>(info) {}

    void init(Osmium::OSM::Meta& meta) {
        fprintf(stderr, "softcut second-pass init\n");

        if(debug) {
            fprintf(stderr, "\n\n===== NODES =====\n\n");
        } else {
            pg.init(meta);
        }
    }


    void node(Osmium::OSM::Node *node) {
        if(debug) {
            fprintf(stderr, "softcut node %d v%d\n", node->id(), node->version());
        } else {
            pg.node(node);
        }
    }

    void after_nodes() {
        if(debug) {
            fprintf(stderr, "after nodes\n");
            fprintf(stderr, "\n\n===== WAYS =====\n\n");
        } else {
            pg.after_nodes();
        }
    }


    void way(Osmium::OSM::Way *way) {
        if(debug) {
            fprintf(stderr, "softcut way %d v%d\n", way->id(), way->version());
        } else {
            pg.way(way);
        }
    }

    void after_ways() {
        if(debug) {
            fprintf(stderr, "after ways\n");
            fprintf(stderr, "\n\n===== RELATIONS =====\n\n");
        }
        else {
            pg.after_ways();
        }
    }


    void relation(Osmium::OSM::Relation *relation) {
        if(debug) {
            fprintf(stderr, "softcut relation %d v%d\n", relation->id(), relation->version());
        } else {
            pg.relation(relation);
        }
    }

    void after_relations() {
        if(debug) {
            fprintf(stderr, "after relations\n");
        } else {
            pg.after_relations();
        }
    }

    void final() {
        fprintf(stderr, "softcut second-pass finished\n");
        if(!debug) {
            pg.final();
        }
    }
};

#endif // SPLITTER_SOFTCUT_HPP

