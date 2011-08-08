'''
Created on 17.06.2010

@author: morgenro
'''

import os

class Reportable(object):
    
    def __init__(self, name, desc):
        self.name = name
        self.description = desc
        self.reportitems = []
        self.graphs = []
        self.text = None
        self.fail = False
        self.reportfiles = []
        
    def addReportFile(self, hostname, file, name, description):
        f = ReportFile(hostname, file, name, description)
        self.reportfiles.append(f)
        
    def defineGraph(self, identifier, name, description, xlabel = None, ylabel = None, persistent = False, xdate = False):
        g = ReportGraph(identifier, name, description, xlabel, ylabel, persistent, xdate)
        self.graphs.append(g)
        
    def addGraphableData(self, identifier, x, y):
        for g in self.graphs:
            if g.identifier == identifier:
                g.addData(x, y)
                
    def printReport(self, text):
        if self.text == None:
            self.text = text + "\n"
        else:
            self.text = self.text + text + "\n"
            
class ReportFile(object):
    
    def __init__(self, hostname, file, name, description):
        self.hostname = hostname
        self.file = file
        self.name = name
        self.description = description

class ReportGraph(object):
    
    def __init__(self, identifier, name, description, xlabel = None, ylabel = None, persistent = False, xdate = False):
        self.identifier = identifier
        self.name = name
        self.description = description
        self.data = []
        self.xlabel = xlabel
        self.ylabel = ylabel
        self.persistent = persistent
        self.xdate = xdate
        
    def addData(self, x, y):
        self.data.append( [ x, y ] )
        
#    def load(self, filename):
#        f = open(filename, 'r')
#        
#        for line in f.readlines():
#            line = line.replace("\n","")
#            data = line.split(" ")
#            self.data.insert(0, [ data[0], data[1] ])
#            
#        f.close()
        
    def save(self, filename):
        if self.persistent:
            f = open(filename, 'a')
        else:
            f = open(filename, 'w')
            
        for d in self.data:
            f.write(str(d[0]) + " " + str(d[1]) + "\n")
            
        f.close()
        
    def plot(self, prefix):
        self.save(prefix + ".dat")
        plotimage = prefix + ".png"
        plotfile = prefix + ".plot"
        
        try:
            os.remove(plotimage)
        except:
            pass
        
        try:
            os.remove(plotfile)
        except:
            pass
        
        f = open(plotfile, 'w')
        f.write("set terminal png font \"/usr/share/fonts/truetype/msttcorefonts/Arial.ttf\" 10 size 1024,320\n")
        #f.write("set title \"" + self.name + "\"\n")
        
        if self.ylabel != None:
            f.write("set ylabel \"" + self.ylabel + "\"\n")
            
        if self.xlabel != None:
            f.write("set xlabel \"" + self.xlabel + "\"\n")
            
        if self.xdate:
            f.write("set xdata time\n")
            f.write("set timefmt \"%Y-%m-%d\ %H:%M:%S\n")

        f.write("set output \""+ plotimage + "\"\n")
        
        if self.xdate:
            f.write("plot '" + prefix + ".dat" + "' using 1:3 title '" + self.name + "' with linespoints\n")
        else:
            f.write("plot '" + prefix + ".dat" + "' using 1:2 title '" + self.name + "' with linespoints\n")
        f.close()
        
        os.system("gnuplot " + plotfile)
        
        return plotimage

class ReportItem(object):
    
    def __init__(self, name, desc, value):
        self.name = name
        self.description = desc
        self.value = value

class BasicReport(object):
    
    def __init__(self, reportables):
        self.reports = reportables
        pass

    def gen(self):
        pass