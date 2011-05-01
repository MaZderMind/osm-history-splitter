#include <osmium/output/xml.hpp>

class Cut : public Osmium::Handler::Base {
    
protected:
    
    struct bbox {
        std::string name;
        double x1, y1, x2, y2;
        Osmium::Output::XML *writer;
        
        bool enabled;
    };
    
    std::vector<bbox> bboxes;
    
    ~Cut() {
        for(int i=0, l = bboxes.size(); i<l; i++) {
            delete bboxes[i].writer;
        }
    }
    
public:
    
    bool debug;
    
    void addBbox(std::string name, double x1, double y1, double x2, double y2) {
        Osmium::Output::XML *writer = new Osmium::Output::XML(name.c_str());
        writer->writeVisibleAttr = true; // enable visible attribute
        writer->writeBounds(x1, y1, x2, y2);
        
        bbox b = {
            /* name */ name, 
            /* coords */ x1, y1, x2, y2, 
            /* writer */ writer, 
            /* enabled */ false
           };
        bboxes.push_back(b);
    }
};

