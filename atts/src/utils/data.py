'''    
Created on 11.06.2010

@author: morgenro
'''

import os
import ConfigParser
from utils import ssh
import threading

hosts = None

def getHosts():
    global hosts
    
    if hosts != None:
        return hosts

    hosts = []
    
    for h in os.listdir("data/hosts"):
        if not h.startswith("."):
            host = Host("data/hosts/" + h)
            
            if not host.disabled:
                hosts.append( host )
        
    return hosts

class DelayedExecution(threading.Thread):
    
    def __init__(self, host, command):
        threading.Thread.__init__(self)
        self.host = host
        self.command = command
    
    def run(self):
        self.host.execute(self.command)

class Host(object):
    
    def __init__(self, filename):
        ''' read the configuration of this host '''
        self.config = ConfigParser.RawConfigParser()
        self.config.read(filename);
        
        self.name = self.config.get("main", "name")
        self.address = self.config.get("network", "ip")
        
        try:
            self.disabled = (self.config.get("main", "disable") == "1")
        except ConfigParser.NoOptionError:
            self.disabled = False

    def activate(self):
        self.ssh = ssh.RemoteHost(self.address, "root", None, "data/keys/id_dsa")
        self.ssh.connect()
        pass
        
    def deactivate(self):
        if self.ssh != None:
            self.ssh.close()
        pass
    
    def put(self, local, remote):
        self.ssh.put(local, remote)
        
    def get(self, remote, local):
        self.ssh.get(remote, local)
        
    def execute(self, command):
        self.ssh.execute(command)
        
    def remove(self, filename):
        self.ssh.execute("/bin/rm " + filename)
