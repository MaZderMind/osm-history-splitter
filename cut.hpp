class Cut : public Osmium::Handler::Base {
    
protected:
    
    struct bbox {
        std::string name;
        double x1, y1, x2, y2;
    };
    
    std::vector<bbox> bboxes;
    
public:
    void addBbox(std::string name, double x1, double y1, double x2, double y2) {
        bbox bbox = { name, x1, y1, x2, y2 };
        bboxes.push_back(bbox);
    }
};

