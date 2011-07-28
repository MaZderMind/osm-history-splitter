#!/usr/bin/python
import sys, os, tempfile

 # the directory to scan for clipbounds-files
clipDir = "clipbounds"

# the type of clipbounds to use (OSM or POLY)
clipType = "POLY" # OSM
clipExtension = ".poly" # .osm

# the clipbounds are read from the directory and created in a hirachy, so
#  clipbounds/europe.poly            is read from the planetfile
#  clipbounds/asia.poly              is read from the planetfile
#  clipbounds/europe/germany.poly    is read from the generated europe.osm.pbf
#  clipbounds/europe/italy.poly      is read from the generated europe.osm.pbf
#  clipbounds/foo/bar.poly           is read from the planetfile, because there was no foo.poly

# the desired result datatype (.osm.pbf, .osh.pbf, .osm, .osh, ...)
dataType = ".osm.pbf"

# the maximum number of parallel running extracts
# this is ( <your systems memory in GB> - 1) * 1024 / <size per extract>
# where <size per extract> is 190 MB for Hardcut and 350 MB for Softcut
#
# to achive best results, set maxProcesses = 1 and start with a low value
# for maxParallel, the run the tool a little. increase maxParallel,
# re-run the tool for some seconds, ...
# when creating the last bit-vector takes more time then creating the
# first vectors, the os starts to swap the bit-vectors out. reduce the
# number by one and try again
maxParallel = 9

# the number of parallel extracts is determined by the available memory.
# when all bit-vectory fit into the RAM, runtime is mostly a matter of CPU.
# if you have multiple cores, you can split quicker by distributing the
# point-in-polygon tests about your cores. This increases the number of
# disk-seeks, because multiple processes tries to access the same file,
# but in most cases this should not hit the performance much.
maxProcesses = 4

# on my PC (4 GB, 4 Cores) i achived best results when doing 9 extracts
# in parallel with 4 processes.

# the source file
inputFile = "/home/peter/osm-data/planet-latest.osm.pbf"

# the directory to place the generated extracts into
outputDir = "/home/peter/osm-history-splitter/o"

# path to the compiled splitter
splitterCommand = "/home/peter/osm-history-splitter/osm-history-splitter"


def process(tasks):
    (source, foo) = os.path.split(tasks[0])
    if(source == ""):
        source = inputFile
    else:
        source = outputDir + "/" + source + dataType

    if not os.path.exists(source):
        source = inputFile

    print "splitting", source, "to", tasks

    if(sys.argv.count("--plan") > 0):
        return

    (fp, configfile) = tempfile.mkstemp()
    os.write(fp, "# auto-generated\n")
    for task in tasks:
        dest = os.path.join(outputDir, task + dataType)
        dirname = os.path.dirname(dest)

        if not os.path.exists(dirname):
            print "Creating", dirname
            os.mkdir(dirname)

        os.write(fp, dest)
        os.write(fp, "\t")
        os.write(fp, clipType)
        os.write(fp, "\t")
        os.write(fp, clipDir + "/" + task + clipExtension)
        os.write(fp, "\n")
    
    os.close(fp)
    os.spawnl(os.P_WAIT, splitterCommand, splitterCommand, "--softcut", source, configfile)
    os.unlink(configfile)


tasks = []
lastdir = "";
stack = [clipDir]
while stack:
    directory = stack.pop(0)
    for base in sorted(os.listdir(directory)):
        name = os.path.join(directory, base)
        if os.path.isdir(name):
            if not os.path.islink(name):
                stack.append(name)
        else:
            if name.endswith(clipExtension):
                name = os.path.relpath(name, clipDir)
                (name, ext) = os.path.splitext(name)

                if len(tasks) > 0 and (lastdir != directory or len(tasks) == maxParallel):
                    process(tasks)
                    tasks = []

                lastdir = directory
                tasks.append(name)

