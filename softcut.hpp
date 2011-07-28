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
       - if the way-node is recorded in the bboxes node-tracker
         - record its id in the bboxes way-id-tracker

     - if its id is in the bboxes way-tracker
       - walk over all way-nodes
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
 - needs more RAM: 350 MB per BBOX
   - ((1400000000÷8)+(1400000000÷8)+(130000000÷8)+(1500000÷8))÷1024÷1024 MB
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

    // - walk over all node-versions
    //   - walk over all bboxes
    //     - if the current node-version is inside the bbox
    //       - record its id in the bboxes node-tracker
    void node(Osmium::OSM::Node *node) {
        if(debug) {
            fprintf(stderr, "softcut node %d v%d\n", node->id(), node->version());
        } else {
            pg.node(node);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];
            if(extract->contains(node)) {
                if(debug) fprintf(stderr, "node is in extract [%d], recording in node_tracker\n", i);

                if((int)extract->node_tracker.size() < node->id()) {
                    fprintf(stderr, "WARNING! node_tracker is too small to hold id %d, resizing...\n", node->id());
                    fprintf(stderr, "    TIP: increase estimation of max. node id in cut.hpp\n");
                    extract->node_tracker.reserve(node->id());
                }

                extract->node_tracker[node->id()] = true;
            }
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

    // - walk over all way-versions
    //   - walk over all bboxes
    //     - walk over all way-nodes
    //       - if the way-node is recorded in the bboxes node-tracker
    //         - record its id in the bboxes way-id-tracker
    //
    //     - if its id is in the bboxes way-tracker
    //       - walk over all way-nodes
    //         - record its id in the bboxes extra-node-tracker
    void way(Osmium::OSM::Way *way) {
        if(debug) {
            fprintf(stderr, "softcut way %d v%d\n", way->id(), way->version());
        } else {
            pg.way(way);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            bool hit = false;
            SoftcutExtractInfo *extract = info->extracts[i];
            Osmium::OSM::WayNodeList nodes = way->nodes();

            for(int ii = 0, ll = nodes.size(); ii<ll; ii++) {
                Osmium::OSM::WayNode node = nodes[ii];
                if(extract->node_tracker[node.ref()]) {
                    if(debug) fprintf(stderr, "way has a node (%d) inside extract [%d], recording in way_tracker\n", node.ref(), i);
                    hit = true;

                    if((int)extract->way_tracker.size() < way->id()) {
                        fprintf(stderr, "WARNING! way_tracker is too small to hold id %d, resizing...\n", way->id());
                        fprintf(stderr, "    TIP: increase estimation of max. node id in cut.hpp\n");
                        extract->way_tracker.reserve(way->id());
                    }

                    extract->way_tracker[way->id()] = true;
                    break;
                }
            }

            if(hit) {
                if(debug) fprintf(stderr, "also recording the extra nodes of the way in the extra_node_tracker: \n\t");
                for(int ii = 0, ll = nodes.size(); ii<ll; ii++) {
                    Osmium::OSM::WayNode node = nodes[ii];
                    if(debug) fprintf(stderr, "%d ", node.ref());

                    if((int)extract->extra_node_tracker.size() < node.ref()) {
                        fprintf(stderr, "WARNING! extra_node_tracker is too small to hold id %d, resizing...\n", node.ref());
                        fprintf(stderr, "    TIP: increase estimation of max. node id in cut.hpp\n");
                        extract->extra_node_tracker.reserve(node.ref());
                    }

                    extract->extra_node_tracker[node.ref()] = true;
                }
                if(debug) fprintf(stderr, "\n");
            }
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

    // - walk over all relation-versions
    //   - walk over all bboxes
    //     - walk over all relation-members
    //       - if the relation-member is recorded in the bboxes node- or way-tracker
    //         - record its id in the bboxes relation-tracker
    void relation(Osmium::OSM::Relation *relation) {
        if(debug) {
            fprintf(stderr, "softcut relation %d v%d\n", relation->id(), relation->version());
        } else {
            pg.relation(relation);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];
            Osmium::OSM::RelationMemberList members = relation->members();
            
            for(int ii = 0, ll = members.size(); ii<ll; ii++) {
                Osmium::OSM::RelationMember member = members[ii];
                if(debug) fprintf(stderr, "relation has a member (%c %d) inside extract [%d], recording in relation_tracker\n", member.type(), member.ref(), i);

                if( (member.type() == 'n' && extract->node_tracker[member.ref()]) || (member.type() == 'w' && extract->way_tracker[member.ref()]) ) {

                    if((int)extract->relation_tracker.size() < relation->id()) {
                        fprintf(stderr, "WARNING! relation_tracker is too small to hold id %d, resizing...\n", relation->id());
                        fprintf(stderr, "    TIP: increase estimation of max. node id in cut.hpp\n");
                        extract->relation_tracker.reserve(relation->id());
                    }

                    extract->relation_tracker[relation->id()] = true;
                    break;
                }
            }
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
        if(!debug) {
            pg.final();
        }

        fprintf(stderr, "softcut first-pass finished\n");
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

    // - walk over all node-versions
    //   - walk over all bboxes
    //     - if the node-id is recorded in the bboxes node-tracker or in the extra-node-tracker
    //       - send the node to the bboxes writer
    void node(Osmium::OSM::Node *node) {
        if(debug) {
            fprintf(stderr, "softcut node %d v%d\n", node->id(), node->version());
        } else {
            pg.node(node);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];

            if(extract->node_tracker[node->id()] || extract->extra_node_tracker[node->id()])
                extract->writer->node(node);
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

    // - walk over all way-versions
    //   - walk over all bboxes
    //     - if the way-id is recorded in the bboxes way-tracker
    //       - send the way to the bboxes writer
    void way(Osmium::OSM::Way *way) {
        if(debug) {
            fprintf(stderr, "softcut way %d v%d\n", way->id(), way->version());
        } else {
            pg.way(way);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];

            if(extract->way_tracker[way->id()])
                extract->writer->way(way);
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

    // - walk over all relation-versions
    //   - walk over all bboxes
    //     - if the relation-id is recorded in the bboxes relation-tracker
    //       - send the relation to the bboxes writer
    void relation(Osmium::OSM::Relation *relation) {
        if(debug) {
            fprintf(stderr, "softcut relation %d v%d\n", relation->id(), relation->version());
        } else {
            pg.relation(relation);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];

            if(extract->relation_tracker[relation->id()])
                extract->writer->relation(relation);
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
        if(!debug) {
            pg.final();
        }

        fprintf(stderr, "softcut second-pass finished\n");
    }
};

#endif // SPLITTER_SOFTCUT_HPP

