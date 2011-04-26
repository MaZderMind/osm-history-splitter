#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <getopt.h>
#include <unistd.h>

#define OSMIUM_MAIN
#include <osmium.hpp>

//#include "softcut.hpp"
#include "hardcut.hpp"

bool readConfig(char *conffile, Cut *cutter);

int main(int argc, char *argv[]) {
    bool softcut = false;
    bool debug = false;
    char *filename, *conffile;
    
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
    
    if (optind > argc-2) {
        fprintf(stderr, "Usage: %s [OPTIONS] OSMFILE CONFIGFILE\n", argv[0]);
        return 1;
    }
    
    filename = argv[optind];
    conffile = argv[optind+1];
    
    if(softcut & !strcmp(filename, "-")) {
        fprintf(stderr, "Can't read from stdin when in softcut\n");
        return 1;
    }
    
    Osmium::Framework osmium(debug);
    
    if(softcut) {
        //Hardcut *cutter = new Hardcut();
        //readConfig(conffile, cutter);
        //
        //osmium.parse_osmfile<SoftcutPass1>(filename, cutter);
        //osmium.parse_osmfile<SoftcutPass2>(filename, cutter);
        //
        //delete cutter;
        
        fprintf(stderr, "Softcut is not yet implemented\n");
        return 1;
    } else {
        Hardcut *cutter = new Hardcut();
        if(!readConfig(conffile, cutter))
        {
            fprintf(stderr, "error reading config\n");
            return 1;
        }
        
        osmium.parse_osmfile<Hardcut>(filename, cutter);
        delete cutter;
    }
    
    return 0;
}

bool readConfig(char *conffile, Cut *cutter)
{
    const int linelen = 4096;
    
    FILE *fp = fopen(conffile, "r");
    if(!fp)
    {
        fprintf(stderr, "unable to open config file %s\n", conffile);
        return false;
    }
    
    char line[linelen];
    while(fgets(line, linelen-1, fp))
    {
        line[linelen-1] = '\0';
        if(line[0] == '#' || line[0] == '\r' || line[0] == '\n' || line[0] == '\0')
            continue;
        
        int n = 0;
        char *tok = strtok(line, "\t ");
        
        const char *name;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        
        while(tok)
        {
            switch(n)
            {
                case 0:
                    name = tok;
                    break;
                
                case 1:
                    if(0 != strcmp("BBOX", tok))
                    {
                        fprintf(stderr, "output %s of type %s: unkown output type\n", name, tok);
                        return false;
                    }
                    break;
                
                case 2:
                    x1 = atof(tok);
                    break;
                
                case 3:
                    y1 = atof(tok);
                    break;
                
                case 4:
                    x2 = atof(tok);
                    break;
                
                case 5:
                    y2 = atof(tok);
                    break;
            }
            
            tok = strtok(NULL, "\t ");
            n++;
        }
        
        cutter->addBbox(name, x1, y1, x2, y2);
    }
    fclose(fp);
    return true;
}

