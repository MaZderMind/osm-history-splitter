#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <getopt.h>
#include <unistd.h>

#define OSMIUM_MAIN
#include <osmium.hpp>

#include <geos/geom/MultiPolygon.h>
#include <geos/algorithm/locate/IndexedPointInAreaLocator.h>

#include "softcut.hpp"
#include "hardcut.hpp"

template <class TExtractInfo> bool readConfig(char *conffile, Cut<TExtractInfo> *cutter);

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
        Softcut *cutter = new Softcut();
        cutter->debug = debug;
        if(!readConfig(conffile, cutter))
        {
            fprintf(stderr, "error reading config\n");
            return 1;
        }

        Osmium::OSMFile* infile = new Osmium::OSMFile(filename);
        cutter->phase = Softcut::PHASE::ONE;
        infile->read<Softcut>(cutter);

        cutter->phase = Softcut::PHASE::TWO;
        infile->read<Softcut>(cutter);

        delete cutter;
    } else {
        Hardcut *cutter = new Hardcut();
        cutter->debug = debug;
        if(!readConfig(conffile, cutter))
        {
            fprintf(stderr, "error reading config\n");
            return 1;
        }

        Osmium::OSMFile* infile = new Osmium::OSMFile(filename);
        infile->read<Hardcut>(cutter);
        delete infile;

        delete cutter;
    }

    return 0;
}

template <class TExtractInfo> bool readConfig(char *conffile, Cut<TExtractInfo> *cutter) {
    const int linelen = 4096;

    FILE *fp = fopen(conffile, "r");
    if(!fp) {
        fprintf(stderr, "unable to open config file %s\n", conffile);
        return false;
    }

    char line[linelen];
    while(fgets(line, linelen-1, fp)) {
        line[linelen-1] = '\0';
        if(line[0] == '#' || line[0] == '\r' || line[0] == '\n' || line[0] == '\0')
            continue;

        int n = 0;
        char *tok = strtok(line, "\t ");

        const char *name = NULL;
        double minlon = 0, minlat = 0, maxlon = 0, maxlat = 0;
        char type = '\0';

        while(tok) {
            switch(n) {
                case 0:
                    name = tok;
                    break;

                case 1:
                    if(0 == strcmp("BBOX", tok))
                        type = 'b';
                    else if(0 == strcmp("POLY", tok))
                        type = 'p';
                    else {
                        type = '\0';
                        fprintf(stderr, "output %s of type %s: unkown output type\n", name, tok);
                        return false;
                    }
                    break;

                case 2:
                    switch(type) {
                        case 'b':
                            if(4 == sscanf(tok, "%lf,%lf,%lf,%lf", &minlon, &minlat, &maxlon, &maxlat)) {
                                geos::geom::Geometry *geom = Osmium::GeometryReader::fromBBox(minlon, minlat, maxlon, maxlat);
                                cutter->addExtract(name, geom);
                            }
                            break;
                        case 'p':
                            char file[linelen];
                            if(1 == sscanf(tok, "%s", file)) {
                                geos::geom::Geometry *geom = Osmium::GeometryReader::fromPolyFile(file);
                                cutter->addExtract(name, geom);
                            }
                            break;
                    }
                    break;
            }

            tok = strtok(NULL, "\t ");
            n++;
        }
    }
    fclose(fp);
    return true;
}
