'''
Created on 11.06.2010

@author: morgenro
'''

import hashlib
from report.base import Reportable 

class BaseTestcase(Reportable):
    
    def __init__(self, name, description = None):
        Reportable.__init__(self, name, description)
        self.hosts = []
        pass
    
    def prepare(self):
        pass
    
    def run(self):
        pass

    def cleanup(self):
        pass
    
    def md5(self, fileName, excludeLine="", includeLine=""):
        """Compute md5 hash of the specified file"""
        m = hashlib.md5()
        try:
            fd = open(fileName,"rb")
        except IOError:
            print "Unable to open the file in readmode:", fileName
            return
        content = fd.readlines()
        fd.close()
        for eachLine in content:
            if excludeLine and eachLine.startswith(excludeLine):
                continue
            m.update(eachLine)
        m.update(includeLine)
        return m.hexdigest()
    
    def collectSyslog(self, hosts):
        for host in hosts:
            host.execute("/sbin/logread >> /tmp/logread.log")
            
        self.reportFilename(hosts, "/tmp/logread.log", "System Log")
        
        for host in hosts:
            host.remove("/tmp/logread.log")
        
    
    def reportFilename(self, hosts, file, description):
        for host in hosts:
            local_path = "data/tmp/" + self.name + "." + host.name + "." + file.replace("/", ".")
            host.get(file, local_path)
            self.addReportFile(host.name, local_path, file, description)
    
    def getFileContent(self, host, file):
        local_path = "data/tmp/" + self.name + "." + host.name + "." + file.replace("/", ".")
        host.get(file, local_path)
        f = open(local_path, 'r')
        content = f.read()
        f.close()
        return content
    
    def activateHosts(self):
        for host in self.hosts:
            host.activate()
            
    def deactivateHosts(self):
        for host in self.hosts:
            host.deactivate()
            
    def putHosts(self, local, remote):
        for host in self.hosts:
            host.put(local, remote)
            
    def removeHosts(self, remove):
        for host in self.hosts:
            host.remove(remove)
            
    def executeHosts(self, command):
        for host in self.hosts:
            host.execute(command)
            
    def saveUCI(self, package):
        """ save the current configuration """
        self.executeHosts("/sbin/uci export " + package + " > /tmp/saved-" + package + ".config")
        
    def restoreUCI(self, package):
        """ restore the previous configuration for this package """
        self.executeHosts("/bin/cat /tmp/saved-" + package + ".config | /sbin/uci import " + package)

        """ remove the saved configuration """
        self.removeHosts("/tmp/saved-" + package + ".config")
