#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <getopt.h>
#include <unistd.h>

#define OSMIUM_MAIN
#include <osmium.hpp>

//#include "softcut.hpp"
#include "hardcut.hpp"

int main(int argc, char *argv[]) {
    bool softcut = false;
    bool debug = false;
    char *filename;
    
    static struct option long_options[] = {
        {"debug",               no_argument, 0, 'd'},
        {"softcut",             no_argument, 0, 's'},
        {"hardcut",             no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    while (1) {
        int c = getopt_long(argc, argv, "dsh", long_options, 0);
        if (c == -1)
            break;
        
        switch (c) {
            case 'd':
                debug = true;
                break;
            case 's':
                softcut = true;
                break;
            case 'h':
                softcut = false;
                break;
        }
    }
    
    if (optind == argc-1) {
        filename = argv[optind];
    } else {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE" << std::endl;
        return 1;
    }
    
    if(softcut & !strcmp(filename, "-")) {
        std::cerr << "Can't read from stdin when in softcut" << std::endl;
        return 1;
    }
    
    Osmium::Framework osmium(debug);
    
    if(softcut) {
        //osmium.parse_osmfile<SoftcutPass1>(filename, new Hardcut());
        //osmium.parse_osmfile<SoftcutPass2>(filename, new Hardcut());
        std::cerr << "Softcut is not yet implemented" << std::endl;
        return 1;
    } else {
        Hardcut *cut = new Hardcut();
        cut->addBbox("world", -180, -90, 180, 90);
        
        osmium.parse_osmfile<Hardcut>(filename, cut);
        delete cut;
    }
    
    return 0;
}

