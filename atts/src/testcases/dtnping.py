'''
Created on 27.10.2010

@author: morgenro
'''

import Base
from utils import data
import os

class SimplePing(Base.BaseTestcase):
    '''
    classdocs
    '''

    def __init__(self):
        '''
        Constructor
        '''
        Base.BaseTestcase.__init__(self, "ping01", "A simple dtn ping between two nodes with wireless network.")
        hosts = data.getHosts()
        
        """ pick two host out of the list """
        self.hosts.append(hosts[0])
        self.hosts.append(hosts[1])
        
    def prepare(self):
        """ activate the hosts, this starts a ssh session to the hosts """
        self.activateHosts()
        
        """ copy wait scripts to the hosts """
        self.putHosts("data/scripts/waitfor-dtndaemon.sh", "/tmp/waitfor-dtndaemon.sh")
        
        """ save the configuration for IBR-DTN """
        self.saveUCI("ibrdtn")
        
        """ configure the IBR-DTN software for testing purpose """
        self.putHosts("data/config/ibrdtn-config.uci", "/tmp/test-ibrdtn.config")

        """ restore the previous configuration for IBR-DTN """
        self.executeHosts("/bin/cat /tmp/test-ibrdtn.config | /sbin/uci import ibrdtn")
        
        """ restart the DTN Daemons on all nodes """
        self.restartDaemon()
    
    def run(self):
        """ send the bundle """
        self.hosts[0].execute("/usr/bin/dtnping --count 10 --lifetime 60 dtn://" + self.hosts[1].name + "/echo > /tmp/echo-output.log 2>&1")
        
        """ add output to the report """
        self.printReport( self.getFileContent(self.hosts[0], "/tmp/echo-output.log") )
    
    def cleanup(self):
        """ delete the result of dtnping """
        self.hosts[0].remove("/tmp/echo-output.log")
        
        """ delete the testing configuration """
        self.removeHosts("/tmp/test-ibrdtn.config")
        
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