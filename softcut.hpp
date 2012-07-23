#ifndef SPLITTER_SOFTCUT_HPP
#define SPLITTER_SOFTCUT_HPP

#include "cut.hpp"

/*

Softcut Algorithm
 - walk over all node-versions
   - walk over all bboxes
     - if the current node-version is inside the bbox
       - record its id in the bboxes node-tracker

 - initialize the current-way-id to 0
 - walk over all way-versions
   - if current-way-id != 0 and current-way-id != the id of the currently iterated way (in other words: this is a different way)
     - walk over all bboxes
       - if the way-id is in the bboxes way-id-tracker (in other words: the way is in the output)
         - append all nodes of the current-way-nodes set to the extra-node-tracker
     - clear the current-way-nodes set
   - update the current-way-id
   - walk over all way-nodes
     - append the node-ids to the current-way-nodes set
   - walk over all bboxes
     - walk over all way-nodes
       - if the way-node is recorded in the bboxes node-tracker
         - record its id in the bboxes way-id-tracker

- after all ways
  - walk over all bboxes
    - if the way-id is in the bboxes way-id-tracker (in other words: the way is in the output)
      - append all nodes of the current-way-nodes set to the extra-node-tracker

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
    growing_bitset node_tracker;
    growing_bitset extra_node_tracker;
    growing_bitset way_tracker;
    growing_bitset relation_tracker;

    SoftcutExtractInfo(std::string name) : ExtractInfo(name) {}
};

class SoftcutInfo : public CutInfo<SoftcutExtractInfo> {

public:
    std::multimap<osm_object_id_t, osm_object_id_t> cascading_relations_tracker;
};


class SoftcutPassOne : public Cut<SoftcutInfo> {
private:
    osm_object_id_t current_way_id;
    typedef std::set<osm_object_id_t> current_way_nodes_t;
    typedef std::set<osm_object_id_t>::iterator current_way_nodes_it;
    current_way_nodes_t current_way_nodes;

    // - walk over all bboxes
    //   - if the way-id is in the bboxes way-id-tracker (in other words: the way is in the output)
    //     - append all nodes of the current-way-nodes set to the extra-node-tracker
    void write_way_extra_nodes() {
        if(debug) fprintf(stderr, "finished all versions of way %d, checking for extra nodes\n", current_way_id);
        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];
            if(extract->way_tracker.get(current_way_id)) {
                if(debug) fprintf(stderr, "way had a node inside extract [%d], recording extra nodes\n", i);
                for(current_way_nodes_it it = current_way_nodes.begin(), end = current_way_nodes.end(); it != end; it++) {
                    extract->extra_node_tracker.set(*it);
                    if(debug) fprintf(stderr, "  %d", *it);
                }
                if(debug) fprintf(stderr, "\n");
            }
        }
    }

public:
    SoftcutPassOne(SoftcutInfo *info) : Cut<SoftcutInfo>(info), current_way_id(0), current_way_nodes() {}

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
    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        if(debug) {
            fprintf(stderr, "softcut node %d v%d\n", node->id(), node->version());
        } else {
            pg.node(node);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];
            if(extract->contains(node)) {
                if(debug) fprintf(stderr, "node is in extract [%d], recording in node_tracker\n", i);

                extract->node_tracker.set(node->id());
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

    // - initialize the current-way-id to 0
    // - walk over all way-versions
    //   - if current-way-id != 0 and current-way-id != the id of the currently iterated way (in other words: this is a different way)
    //     - walk over all bboxes
    //       - if the way-id is in the bboxes way-id-tracker (in other words: the way is in the output)
    //         - append all nodes of the current-way-nodes set to the extra-node-tracker
    //     - clear the current-way-nodes set
    //   - update the current-way-id
    //   - walk over all way-nodes
    //     - append the node-ids to the current-way-nodes set
    //   - walk over all bboxes
    //     - walk over all way-nodes
    //       - if the way-node is recorded in the bboxes node-tracker
    //         - record its id in the bboxes way-id-tracker
    //
    // - after all ways
    //   - walk over all bboxes
    //     - if the way-id is in the bboxes way-id-tracker (in other words: the way is in the output)
    //       - append all nodes of the current-way-nodes set to the extra-node-tracker

    void way(const shared_ptr<Osmium::OSM::Way const>& way) {
        // detect a new way
        if(current_way_id != 0 && current_way_id != way->id()) {
            write_way_extra_nodes();
            current_way_nodes.clear();
        }
        current_way_id = way->id();

        if(debug) {
            fprintf(stderr, "softcut way %d v%d\n", way->id(), way->version());
        } else {
            pg.way(way);
        }

        Osmium::OSM::WayNodeList nodes = way->nodes();
        for(int ii = 0, ll = nodes.size(); ii<ll; ii++) {
            Osmium::OSM::WayNode node = nodes[ii];
            current_way_nodes.insert(node.ref());
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];

            for(int ii = 0, ll = nodes.size(); ii<ll; ii++) {
                Osmium::OSM::WayNode node = nodes[ii];
                if(extract->node_tracker.get(node.ref())) {
                    if(debug) fprintf(stderr, "way has a node (%d) inside extract [%d], recording in way_tracker\n", node.ref(), i);

                    extract->way_tracker.set(way->id());
                    break;
                }
            }
        }
    }

    void after_ways() {
        write_way_extra_nodes();
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
    void relation(const shared_ptr<Osmium::OSM::Relation const>& relation) {
        if(debug) {
            fprintf(stderr, "softcut relation %d v%d\n", relation->id(), relation->version());
        } else {
            pg.relation(relation);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            bool hit = false;
            SoftcutExtractInfo *extract = info->extracts[i];
            Osmium::OSM::RelationMemberList members = relation->members();
            
            for(int ii = 0, ll = members.size(); ii<ll; ii++) {
                Osmium::OSM::RelationMember member = members[ii];

                if( !hit && (
                    (member.type() == 'n' && extract->node_tracker.get(member.ref())) ||
                    (member.type() == 'w' && extract->way_tracker.get(member.ref())) ||
                    (member.type() == 'r' && extract->relation_tracker.get(member.ref()))
                )) {

                    if(debug) fprintf(stderr, "relation has a member (%c %d) inside extract [%d], recording in relation_tracker\n", member.type(), member.ref(), i);
                    hit = true;

                    extract->relation_tracker.set(relation->id());
                }

                if(member.type() == 'r') {
                    if(debug) fprintf(stderr, "recording cascading-pair: %d -> %d\n", member.ref(), relation->id());
                    info->cascading_relations_tracker.insert(std::make_pair(member.ref(), relation->id()));
                }
            }

            if(hit) {
                cascading_relations(extract, relation->id());
            }
        }
    }

    void cascading_relations(SoftcutExtractInfo *extract, osm_object_id_t id) {
        typedef std::multimap<osm_object_id_t, osm_object_id_t>::const_iterator mm_iter;

        std::pair<mm_iter, mm_iter> r = info->cascading_relations_tracker.equal_range(id);
        if(r.first == r.second) {
            return;
        }

        for(mm_iter it = r.first; it !=r.second; ++it) {
            if(debug) fprintf(stderr, "\tcascading: %d\n", it->second);

            if(extract->relation_tracker.get(it->second))
                continue;

            extract->relation_tracker.set(it->second);

            cascading_relations(extract, it->second);
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
    void node(const shared_ptr<Osmium::OSM::Node const>& node) {
        if(debug) {
            fprintf(stderr, "softcut node %d v%d\n", node->id(), node->version());
        } else {
            pg.node(node);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];

            if(extract->node_tracker.get(node->id()) || extract->extra_node_tracker.get(node->id()))
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
    void way(const shared_ptr<Osmium::OSM::Way const>& way) {
        if(debug) {
            fprintf(stderr, "softcut way %d v%d\n", way->id(), way->version());
        } else {
            pg.way(way);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];

            if(extract->way_tracker.get(way->id()))
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
    void relation(const shared_ptr<Osmium::OSM::Relation const>& relation) {
        if(debug) {
            fprintf(stderr, "softcut relation %d v%d\n", relation->id(), relation->version());
        } else {
            pg.relation(relation);
        }

        for(int i = 0, l = info->extracts.size(); i<l; i++) {
            SoftcutExtractInfo *extract = info->extracts[i];

            if(extract->relation_tracker.get(relation->id()))
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

