'''
Created on 11.06.2010

@author: morgenro
'''

import Base
from utils import data
from report.base import ReportItem 
import os
import re

class BasicPoettnerReport(Base.BaseTestcase):

    def __init__(self, name, desc):
        Base.BaseTestcase.__init__(self, name, desc)
        self.runs = []
        
        """ define the base data directory """
        self.basedir = "/home/poettner/ibrdtn/atts-report-data"

    def prepare(self):
        """ read the MANIFEST files """
        testruns = os.listdir(self.basedir)
        testruns.sort()
        for r in testruns:
            path = os.path.join(self.basedir, r)
            manifest = ManifestFile(path)
            self.runs.append(manifest)

    def split_string_data(self, data):
        ret = []
        for d in data.split(" "):
            if d != "":
                ret.append(d)
        return ret

    def run(self):
        pass

    def cleanup(self):
        pass

class ManifestFile:
    def __init__(self, path):
        self.path = path
        fd = open(os.path.join(self.path, "MANIFEST"), "r")
        lines = fd.readlines()
        self.timestamp = None
        self.name = None
        self.sender_version = None
        self.receiver_version = None

	try:
	    self.timestamp = lines[0].strip()
	    self.name = lines[1].strip()
	    self.sender_version = lines[4].strip()
	    self.receiver_version = lines[7].strip()
	except IndexError:
	    pass
        
        fd.close()

    def getConfigs(self):
        ret = []
        files = os.listdir(self.path)
        for f in files:
            if f.startswith("config_"):
                ret.append(f)
        ret.sort()
        return ret

    def getThroughputLogs(self):
        ret = []
        files = os.listdir(self.path)
        for f in files:
            if f.startswith("throughput_") and f.endswith(".conf.log"):
                ret.append(f)
        ret.sort()
        return ret

class SummaryReport(BasicPoettnerReport):
    
    def __init__(self):
        BasicPoettnerReport.__init__(self, "SummaryReport", "Greps through the log file and search for errors.")
    
    def run(self):
        for m in self.runs:
            self.printReport(m.name)
            self.printReport("software version recevier: " + str(m.receiver_version))
            self.printReport("software version sender: " + str(m.sender_version))
            c_list = m.getConfigs()
            
            """ report configuration files """
            for c in c_list:
                self.addReportFile(m.name, os.path.join(m.path, c), c, "configuration file")

            for l in m.getThroughputLogs():
                (success, of) = self.check_test(m, l)
                self.printReport(str(success) + " of " + str(of) + " runs successful in " + l)

                warns = self.check_warnings(m, l)
                for w, count in warns.items():
                    self.printReport("WARNING (" + str(count) + "): " + str(w))

                errors = self.check_errors(m, l)
                for e, count in warns.items():
                    self.printReport("ERROR (" + str(count) + "): " + str(e))

            self.printReport("")

    def check_test(self, mf, logfile):
        fd = open(os.path.join(mf.path, logfile), "r")
	state = 0
        run_start = 0
        run_stop = 0
        p_res = None
        t_res = None
	p = re.compile('produced ([0-9]+) bundles of ([0-9]+) bytes in ([\-0-9.]+)[\ ]?s \([0-9]+ lost\)')
        t = re.compile('[0-9.]+ [0-9:]+ \| ([0-9]+) \(([0-9]+)\) bundles of ([0-9]+) bytes in ([\-0-9.]+)[\ ]?s')
        for data in fd.readlines():
            if state == 1:
                t_res = t.match(data)
                if t_res != None:
                    state = 0
                    if p_res.group(1) == t_res.group(1):
                        run_stop = run_stop + 1
                    else:
                        self.printReport("only " + str(t_res.group(1)) + " of " + str(p_res.group(1)) + " bundles with " + str(p_res.group(2)) + " bytes has been transferred")

            res = p.match(data)
            if res != None:
                if state == 1:
                    # previous run failed
                    self.printReport("run " + str(p_res.group(1)) + " bundles with " + str(p_res.group(2)) + " bytes failed")
                p_res = res
                state = 1
                run_start = run_start + 1

        fd.close()
        return (run_stop, run_start)

    def check_warnings(self, mf, logfile):
        # warning dict
        wd = {}
        wre = re.compile('[0-9.]+ [0-9:]+ \| [0-9.]+ WARNING: ([^\n]+)')

        fd = open(os.path.join(mf.path, logfile), "r")
        for data in fd.readlines():
            m = wre.match(data)
            if m != None:
                try:
                    old = wd[m.group(1)]
                except KeyError:
                    old = 0

                wd[m.group(1)] = old + 1
        fd.close()
        return wd

    def check_errors(self, mf, logfile):
        # error dict
        wd = {}
        wre = re.compile('[0-9.]+ [0-9:]+ \| [0-9.]+ ERROR: ([^\n]+)')

        fd = open(os.path.join(mf.path, logfile), "r")
        for data in fd.readlines():
            m = wre.match(data)
            if m != None:
                try:
                    old = wd[m.group(1)]
                except KeyError:
                    old = 0

                wd[m.group(1)] = old + 1
        fd.close()
        return wd

class MemoryReport(BasicPoettnerReport):
    
    def __init__(self):
        BasicPoettnerReport.__init__(self, "MemoryReport", "Track the used memory of all testcases.")
       
    def run(self):
        self.graphmax = 0
        
        for m in self.runs:
            self.printReport(m.name)
            
            """ grep through the throughput logs """
            for l in m.getThroughputLogs():
                self.printReport("scanning " + str(l) + " ...")
                
                fd = open(os.path.join(m.path, l), "r")
                readmem = -1
                sender_or_receiver = 0
                run_count = 0

                self.graphmax = self.graphmax + 2
                self.defineGraph(self.graphmax - 1, "Memory sender [" + m.name + "]", \
                    "Memory usage of the sender for the test run " + m.name + ".\n<br/>Logfile: " + str(l), "run", "memory usage (MB)")
                self.defineGraph(self.graphmax, "Memory receiver [" + m.name + "]", \
                    "Memory usage of the receiver for the test run " + m.name + ".\n<br/>Logfile: " + str(l), "run", "memory usage (MB)")

                for data in fd.readlines():
                    if data.find("Recording free memory") > -1:
                        readmem = 0
                        if data.find("SENDER: ") > -1:
                            sender_or_receiver = -1
                            run_count = run_count + 1
                        else:
                            sender_or_receiver = 0
                    elif readmem != -1:
                        sdata = self.split_string_data(data)
                        if sdata[3] == "Mem:":
                            readmem = readmem + 1
                            self.addGraphableData(self.graphmax + sender_or_receiver, run_count, float(sdata[5]) / 1000)
                        if sdata[3] == "Swap:":
                            readmem = readmem + 1
                        if readmem == 2:
                            readmem = -1
                fd.close()
            self.printReport("")
        pass

class ThroughputReport(BasicPoettnerReport):
    
    def __init__(self):
        BasicPoettnerReport.__init__(self, "ThroughputReport", "Report the duration of transmissions.")
    
    def run(self):
        self.graphmax = 0
        run_count = 0
        
        for m in self.runs:
            self.printReport(m.name)
            
            """ grep through the throughput logs """
            for l in m.getThroughputLogs():
                self.printReport("scanning " + str(l) + " ...")
                previous_size = 0
                
                fd = open(os.path.join(m.path, l), "r")
                readmem = -1

                for data in fd.readlines():
                    if data.find(": Transferred ") > -1:
                        readmem = 0
                    elif readmem != -1:
			try:
                            if data.find(" bundles of ") > -1:
                                sdata = self.split_string_data(data)
                                bundles = int(sdata[3])
                                size = int(sdata[7])
                                duration = sdata[10].strip()
                                duration = float(duration[0:len(duration)-1])
                                throughput = (bundles * size * 8 / 1000000) / duration

                                if size != previous_size:
                                    previous_size = size
                                    self.graphmax = self.graphmax + 1
                                    self.defineGraph(self.graphmax, m.name + ", " + str(bundles) + " bundles with " + \
                                        str(size) + " bytes", "Throughput of transmissions in " + m.name + ".\n<br/>Logfile: " + \
                                        str(l), "run", "throughput (Mbit/s)")
                                    run_count = 0

                                run_count = run_count + 1

                                self.addGraphableData(self.graphmax, run_count, throughput)
                                readmem = -1
			except IndexError:
                            pass
                        except ValueError:
                            pass
                fd.close()
            self.printReport("")
        pass
