'''
Created on 11.06.2010

@author: morgenro
'''

import Base
from utils import data

import subprocess
import re

class UpdateSoftware(Base.BaseTestcase):
    
    def __init__(self):
        Base.BaseTestcase.__init__(self, "UpdateSoftware", "Update the software on the nodes.")
        pass
    
    def run(self):
        hosts = data.getHosts()
        
        for host in hosts:
            if not host.disabled:
                self.update(host)
                self.printReport("host " + host.address + " updated")

    def update(self, host):
        host.activate()
        host.put("data/scripts/update-ibrdtn.sh", "/tmp/udpate-ibrdtn.sh")
        host.execute("/bin/sh /tmp/udpate-ibrdtn.sh")
        host.remove("/tmp/udpate-ibrdtn.sh")
        host.deactivate()
        pass


class ConnectionTest(Base.BaseTestcase):
    
    def __init__(self):
        Base.BaseTestcase.__init__(self, "ConnectionTest", "A simple connection test.")
        self.hosts = data.getHosts()
        pass
    
    def run(self):
        for host in self.hosts:
            if not self.ping(host.address):
                self.printReport("pinging host " + host.address + " failed")
            else:
                self.printReport("pinging host " + host.address + " successful")

    def ping(self, host):
        ping = subprocess.Popen(
            ["ping", "-c", "4", host],
            stdout = subprocess.PIPE,
            stderr = subprocess.PIPE
        )
        
        out, error = ping.communicate()
        matcher = re.compile("([0-9]+) packets transmitted, ([0-9]+) received, (.*)")
        ret = matcher.search(out)
        
        if ret == None:
            self.fail = True
            return False
        else:
            parsed = ret.groups()
            if parsed[0] != parsed[1]:
                self.fail = True
                return False
            
        return True

