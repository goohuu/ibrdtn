'''
Created on 11.06.2010

@author: morgenro
'''

import Base
from utils import data
import time
import datetime
from utils.data import DelayedExecution
from report.base import ReportItem 
import os

class BaseFileTransfer(Base.BaseTestcase):
    
    def __init__(self, name, desc, files):
        Base.BaseTestcase.__init__(self, name, desc)
        self.files = files
        hosts = data.getHosts()
        
        """ pick two host out of the list """
        self.hosts.append(hosts[0])
        self.hosts.append(hosts[1])
        
        """ define the throughput graph """
        self.defineGraph(1, "Throughput", "Throughput of the file transfers", "file size (kbyte)", "kbyte per second")
        
        """ define the daily throughput graph """
        self.defineGraph(2, "Throughput Summarized", "Throughput of the file transfers with all previous results.", "Date/Time", "kbyte per second", persistent=True, xdate=True)
    
    def prepare(self):
        """ activate the hosts, this starts a ssh session to the hosts """
        self.activateHosts()
        
        """ copy wait scripts to the hosts """
        self.putHosts("data/scripts/waitfor-dtndaemon.sh", "/tmp/waitfor-dtndaemon.sh")
        
        """ copy the test payload to host one """
        for filename in self.files:
            self.hosts[0].put(filename, "/tmp/" + os.path.basename(filename))
        
        """ save the configuration for IBR-DTN """
        self.saveUCI("ibrdtn")
    
    def run(self):
        """ container for the results """
        dailyresult = []
        
        for filename in self.files:
            """ basename for the file to transmit """
            basefilename = os.path.basename(filename)
            
            self.printReport("sending file "+ basefilename)
            
            """ start the receiver on host two """
            receiver = DelayedExecution(self.hosts[1], "/usr/bin/dtnrecv --timeout 300 --file /tmp/" + basefilename + " --name ts")
            receiver.start()
            
            s0 = time.time()
            
            """ send the bundle """
            self.hosts[0].execute("/usr/bin/dtnsend --lifetime 300 dtn://" + self.hosts[1].name + "/ts /tmp/" + basefilename)
            
            """ wait until the receiver has returned """
            receiver.join()
            
            """ stop time measurement """
            s1 = time.time()
            
            """ get the received payload file """
            self.hosts[1].get("/tmp/" + os.path.basename(filename), "data/tmp/" + basefilename)
            
            self.printReport("sent file hash: " + str(self.md5(filename)))
            self.printReport("recv file hash: " + str(self.md5("data/tmp/" + basefilename)))
            
            if self.md5(filename) == self.md5("data/tmp/" + basefilename):
                fsize = round(os.path.getsize(filename) / 1000, 2)
                tput = round(fsize / (s1 - s0), 2)
                self.printReport("Time elapsed: " + str(s1 - s0) + " seconds")
                self.printReport("Transfer rate: " + str( fsize / (s1 - s0) ) + " kbytes per second")
                self.addGraphableData(1, str(fsize), str(tput))
                dailyresult.append(tput)
            else:
                self.printReport("transfer failed!")
                self.addGraphableData(1, str(fsize), 0)
                self.fail = True
                
            """ delete the local file """
            os.remove("data/tmp/" + basefilename)

            time.sleep(5)
            
        """ summarize all results to a daily result """
        dailydata = 0
        for d in dailyresult:
            dailydata = dailydata + d
        dailydata = str(round(dailydata / len(dailyresult), 2))
        
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.addGraphableData(2, str(timestamp), dailydata)
            
        """ spend some time waiting until all connections are down in the daemon """
        time.sleep(10)

    def cleanup(self):
        """ delete the test payload on the hosts """
        for filename in self.files:
            self.removeHosts("/tmp/" + os.path.basename(filename))
        
        """ remove wait scripts on the hosts """
        self.removeHosts("/tmp/waitfor-dtndaemon.sh")
        
        """ restore the configuration for IBR-DTN """
        self.restoreUCI("ibrdtn")
        
        """ restart IBR-DTN """
        self.restartDaemon()
        
        """ collect all syslogs """
        self.collectSyslog(self.hosts)
        
        """ deactivate the hosts, this stop the ssh session to the hosts """
        self.deactivateHosts()
        
    def restartDaemon(self):
        """ restart IBR-DTN """
        self.executeHosts("/etc/init.d/ibrdtn restart")
        
        """ wait until all daemons are up """
        self.executeHosts("/bin/sh /tmp/waitfor-dtndaemon.sh")
        
class SimpleFileTransfer(BaseFileTransfer):
    
    def __init__(self):
        files = []
        files.append("data/mocks/random-4M.bin")
        files.append("data/mocks/random-2M.bin")
        files.append("data/mocks/random-1M.bin")
        files.append("data/mocks/random-512k.bin")
        files.append("data/mocks/random-1k.bin")
        BaseFileTransfer.__init__(self, "neighbor01", "A simple file transfer between two nodes with wireless network.", files)
    
    def prepare(self):
        """ call the super prepare """
        BaseFileTransfer.prepare(self)
        
        """ configure the IBR-DTN software for testing purpose """
        self.putHosts("data/config/ibrdtn-config.uci", "/tmp/test-ibrdtn.config")

        """ restore the previous configuration for IBR-DTN """
        self.executeHosts("/bin/cat /tmp/test-ibrdtn.config | /sbin/uci import ibrdtn")
        
        """ restart the DTN Daemons on all nodes """
        self.restartDaemon()
        
    def cleanup(self):
        """ delete the testing configuration """
        self.removeHosts("/tmp/test-ibrdtn.config")

        """ call the super cleanup """
        BaseFileTransfer.cleanup(self)

class WiredFileTransfer(BaseFileTransfer):
    
    def __init__(self):
        files = []
        files.append("data/mocks/random-4M.bin")
        files.append("data/mocks/random-2M.bin")
        files.append("data/mocks/random-1M.bin")
        files.append("data/mocks/random-512k.bin")
        files.append("data/mocks/random-1k.bin")
        BaseFileTransfer.__init__(self, "neighbor02", "Neighbor delivery with wired network.", files)
    
    def prepare(self):
        """ call the super prepare """
        BaseFileTransfer.prepare(self)
        
        """ configure the IBR-DTN software for testing purpose """
        self.putHosts("data/config/ibrdtn-config-wired.uci", "/tmp/test-ibrdtn.config")

        """ restore the previous configuration for IBR-DTN """
        self.executeHosts("/bin/cat /tmp/test-ibrdtn.config | /sbin/uci import ibrdtn")
        
        """ restart the DTN Daemons on all nodes """
        self.restartDaemon()
        
    def cleanup(self):
        """ delete the testing configuration """
        self.removeHosts("/tmp/test-ibrdtn.config")

        """ call the super cleanup """
        BaseFileTransfer.cleanup(self)
        
class EpidemicFileTransfer(BaseFileTransfer):
    
    def __init__(self):
        files = []
        files.append("data/mocks/random-4M.bin")
        files.append("data/mocks/random-2M.bin")
        files.append("data/mocks/random-1M.bin")
        files.append("data/mocks/random-512k.bin")
        files.append("data/mocks/random-1k.bin")
        BaseFileTransfer.__init__(self, "epidemic01", "Epidemic routing with two nodes.", files)
    
    def prepare(self):
        """ call the super prepare """
        BaseFileTransfer.prepare(self)
        
        """ configure the IBR-DTN software for testing purpose """
        self.putHosts("data/config/ibrdtn-config-dynamic.uci", "/tmp/test-ibrdtn.config")

        """ restore the previous configuration for IBR-DTN """
        self.executeHosts("/bin/cat /tmp/test-ibrdtn.config | /sbin/uci import ibrdtn")
        
        """ restart the DTN Daemons on all nodes """
        self.restartDaemon()
        
    def cleanup(self):
        """ delete the testing configuration """
        self.removeHosts("/tmp/test-ibrdtn.config")

        """ call the super cleanup """
        BaseFileTransfer.cleanup(self)
        
class VPNFileTransfer(BaseFileTransfer):
    
    def __init__(self):
        files = []
        files.append("data/mocks/random-4M.bin")
        files.append("data/mocks/random-2M.bin")
        files.append("data/mocks/random-1M.bin")
        files.append("data/mocks/random-512k.bin")
        files.append("data/mocks/random-1k.bin")
        BaseFileTransfer.__init__(self, "vpn01", "Neighbor delivery with VPN network.", files)
    
    def prepare(self):
        """ call the super prepare """
        BaseFileTransfer.prepare(self)
        
        """ configure the IBR-DTN software for testing purpose """
        self.putHosts("data/config/ibrdtn-config-vpn.uci", "/tmp/test-ibrdtn.config")

        """ restore the previous configuration for IBR-DTN """
        self.executeHosts("/bin/cat /tmp/test-ibrdtn.config | /sbin/uci import ibrdtn")
        
        """ restart the DTN Daemons on all nodes """
        self.restartDaemon()
        
    def cleanup(self):
        """ delete the testing configuration """
        self.removeHosts("/tmp/test-ibrdtn.config")

        """ call the super cleanup """
        BaseFileTransfer.cleanup(self)