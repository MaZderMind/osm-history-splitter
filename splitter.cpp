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
geos::geom::Geometry *readPolyFile(char* file);

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

        cutter->phase = Softcut::PHASE::ONE;
        osmium.parse_osmfile<Softcut>(filename, cutter);

        cutter->phase = Softcut::PHASE::TWO;
        osmium.parse_osmfile<Softcut>(filename, cutter);

        delete cutter;
    } else {
        Hardcut *cutter = new Hardcut();
        cutter->debug = debug;
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
                                if(minlon > maxlon) {
                                    double d = maxlon;
                                    minlon = maxlon;
                                    maxlon = d;
                                }

                                if(minlat > maxlat) {
                                    double d = maxlat;
                                    minlat = maxlat;
                                    maxlat = d;
                                }
                                cutter->addBbox(name, minlon, minlat, maxlon, maxlat);
                            }
                            break;
                        case 'p':
                            geos::geom::Geometry *poly = readPolyFile(tok);
                            cutter->addPoly(name, poly);
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

geos::geom::Geometry *readPolyFile(char* file) {
    // shorthand
    geos::geom::GeometryFactory *f = Osmium::global.geos_geometry_factory;

    std::vector<geos::geom::Coordinate> *c;
    std::vector<geos::geom::Geometry*> outer;
    std::vector<geos::geom::Geometry*> inner;

    c = new std::vector<geos::geom::Coordinate>();
    c->push_back(geos::geom::Coordinate(10, 10, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(10, 20, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(20, 20, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(20, 10, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(10, 10, DoubleNotANumber));

    outer.push_back(
        f->createPolygon(
            f->createLinearRing(
                f->getCoordinateSequenceFactory()->create(c)
            ),
            NULL
        )
    );

    c = new std::vector<geos::geom::Coordinate>();
    c->push_back(geos::geom::Coordinate(40, 10, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(40, 20, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(50, 20, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(50, 10, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(40, 10, DoubleNotANumber));
    outer.push_back(
        f->createPolygon(
            f->createLinearRing(
                f->getCoordinateSequenceFactory()->create(c)
            ),
            NULL
        )
    );

    c = new std::vector<geos::geom::Coordinate>();
    c->push_back(geos::geom::Coordinate(9, 12, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(9, 18, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(18, 18, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(18, 12, DoubleNotANumber));
    c->push_back(geos::geom::Coordinate(9, 12, DoubleNotANumber));

    inner.push_back(
        f->createPolygon(
            f->createLinearRing(
                f->getCoordinateSequenceFactory()->create(c)
            ),
            NULL
        )
    );



    geos::geom::MultiPolygon *outerPoly = f->createMultiPolygon(outer);
    geos::geom::MultiPolygon *innerPoly = f->createMultiPolygon(inner);
    geos::geom::Geometry *poly = outerPoly->difference(innerPoly);

    delete outerPoly;
    delete innerPoly;

    /*
    geos::algorithm::locate::IndexedPointInAreaLocator *locator =
        new geos::algorithm::locate::IndexedPointInAreaLocator(*const_cast<const geos::geom::Geometry *>(poly));

    printf("%d %d: %d (left out)\n", 9,  15, locator->locate(new geos::geom::Coordinate( 9, 15, DoubleNotANumber)));
    printf("%d %d: %d (on border)\n", 10,  15, locator->locate(new geos::geom::Coordinate( 10, 15, DoubleNotANumber)));
    printf("%d %d: %d (inside left border)\n", 11, 15, locator->locate(new geos::geom::Coordinate(11, 15, DoubleNotANumber)));
    printf("%d %d: %d (in hole)\n", 15, 15, locator->locate(new geos::geom::Coordinate(15, 15, DoubleNotANumber)));
    printf("%d %d: %d (in hole)\n", 17, 15, locator->locate(new geos::geom::Coordinate(17, 15, DoubleNotANumber)));
    printf("%d %d: %d (inside right border)\n", 19, 15, locator->locate(new geos::geom::Coordinate(19, 15, DoubleNotANumber)));
    printf("%d %d: %d (between shapes)\n", 30, 15, locator->locate(new geos::geom::Coordinate(30, 15, DoubleNotANumber)));
    printf("%d %d: %d (inside right shape)\n", 45, 15, locator->locate(new geos::geom::Coordinate(45, 15, DoubleNotANumber)));
    printf("%d %d: %d (right out)\n", 51, 15, locator->locate(new geos::geom::Coordinate(51, 15, DoubleNotANumber)));
    */

    return poly;
}

