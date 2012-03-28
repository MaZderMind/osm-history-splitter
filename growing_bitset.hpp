#include <iostream>
#include <vector>
#include <memory>

#ifndef GROWING_BITSET_HPP
#define GROWING_BITSET_HPP

class growing_bitset
{
private:
    typedef std::vector<bool> bitvec_t;
    typedef bitvec_t* bitvec_ptr_t;
    typedef std::vector< bitvec_ptr_t > bitmap_t;
    
    
    static const size_t segment_size = 10*1024;
    bitmap_t bitmap;
    
    bitvec_ptr_t find_segment (size_t segment) {
        if (segment >= bitmap.size()) {
            std::cerr << "  resizing ptr-bitmap to " << segment+1 << std::endl;
            bitmap.resize(segment+1);
        }
        
        bitvec_ptr_t ptr = bitmap.at(segment);
        if(!ptr) {
            std::cerr << "  creating bitvec in segment " << segment << std::endl;
            bitmap[segment] = ptr = new std::vector<bool>(segment_size);
        }
        
        return ptr;
    }
    
    bitvec_ptr_t find_segment (size_t segment) const {
        if (segment >= bitmap.size()) {
            std::cerr << "  NOT creating segment " << segment << " due to const operation" << std::endl;
            return NULL;
        }
        
        return bitmap.at(segment);
    }

public:
    ~growing_bitset() {
        for (bitmap_t::iterator it=bitmap.begin(), end=bitmap.end(); it != end; it++) {
            bitvec_ptr_t ptr = (*it);
            if(ptr) delete ptr;
        }
    }
    
    void set(const size_t pos) {
        size_t
            segment = pos / segment_size,
            segmented_pos = pos % segment_size;
        
        std::cerr << "set(" << pos << "): setting bit " << segmented_pos << " in segment " << segment << std::endl;
        
        bitvec_ptr_t bitvec = find_segment(segment);
        bitvec->at(segmented_pos) = true;
    }
    
    bool get (const size_t pos) const {
        size_t
            segment = pos / segment_size,
            segmented_pos = pos % segment_size;
        
        std::cerr << "get(" << pos << "): getting bit " << segmented_pos << " in segment " << segment << std::endl;
        
        bitvec_ptr_t bitvec = find_segment(segment);
        if(!bitvec) return false;
        return (bool)bitvec->at(segmented_pos);
    }
};

#endif
