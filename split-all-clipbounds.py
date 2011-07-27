#!/usr/bin/python
import os, tempfile

# settings
clipDir = "clipbounds"
clipType = "POLY" # OSM
clipExtension = ".poly" # .osm

dataType = ".osm.pbf" # .osh.pbf
maxParallel = 5

#inputFile = "planet-latest.osm.pbf"
inputFile = "/home/peter/rheinland-pfalz.osm.pbf"
outputDir = "/home/peter/osm-history-splitter/o"
splitterCommand = "/home/peter/osm-history-splitter/osm-history-splitter"


def process(tasks):
    (source, foo) = os.path.split(tasks[0])
    if(source == ""):
        source = inputFile
    else:
        source = outputDir + "/" + source + dataType
    
    if not os.path.exists(source):
        source = inputFile
    
    print "Processing", tasks, "from", source
    
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


tasks = []
lastroot = "";
for root, dirs, files in os.walk(clipDir, followlinks=True):
    for name in dirs:
        if(len(tasks) > 0):
            process(tasks)
            tasks = []
    
    for name in files:
        if name.endswith(clipExtension):
            name = os.path.join(root, name)
            name = os.path.relpath(name, clipDir)
            (name, ext) = os.path.splitext(name)
            
            if len(tasks) > 0 and (lastroot != root or len(tasks) == maxParallel):
                process(tasks)
                tasks = []
            
            lastroot = root
            tasks.append(name)

