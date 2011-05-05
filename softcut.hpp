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


class SoftcutBBoxInfo : public BBoxInfo {

public:
    bool enabled;

    std::vector<bool> node_tracker;
    std::vector<bool> extra_node_tracker;
    std::vector<bool> way_tracker;
    std::vector<bool> relation_tracker;

    SoftcutBBoxInfo(std::string name) : BBoxInfo(name) {
        enabled = false;

        fprintf(stderr, "allocating bit-tracker\n");
        node_tracker = std::vector<bool>(BBoxInfo::est_max_node_id);
        extra_node_tracker = std::vector<bool>(BBoxInfo::est_max_node_id);
        way_tracker = std::vector<bool>(BBoxInfo::est_max_way_id);
        relation_tracker = std::vector<bool>(BBoxInfo::est_max_relation_id);
    }
};

class Softcut : public Cut<SoftcutBBoxInfo> {

public:

    enum PHASE {ONE, TWO};
    PHASE phase;

    void callback_init() {
        switch(phase) {
            case ONE: return one_callback_init();
            case TWO: return two_callback_init();
        }
    }

    void callback_final() {
        switch(phase) {
            case ONE: return one_callback_final();
            case TWO: return two_callback_final();
        }
    }

    void one_callback_init() {
        fprintf(stderr, "softcut pass 1 init\n");
        for(int i = 0, l = bboxes.size(); i<l; i++) {
            fprintf(stderr, "\tbbox[%d] %s\n", i, bboxes[i]->name.c_str());
        }
    }

    void one_callback_final() {
        fprintf(stderr, "softcut pass 1 finished\n");
    }

    void two_callback_init() {
        fprintf(stderr, "softcut pass 2 init\n");
    }

    void two_callback_final() {
        fprintf(stderr, "softcut pass 2 finished\n");
    }
};

#endif // SPLITTER_SOFTCUT_HPP

