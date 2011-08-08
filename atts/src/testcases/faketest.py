'''
Created on 11.06.2010

@author: morgenro
'''

import Base
from utils import data
from report.base import ReportItem 

class FakeTestFailed(Base.BaseTestcase):
    
    def __init__(self):
        Base.BaseTestcase.__init__(self, "FakeTestFailed", "This is just a fake test to prove the report modules.")
        hosts = data.getHosts()
        
        """ pick two host out of the list """
        self.hosts.append(hosts[0])
        self.hosts.append(hosts[1])
    
    def prepare(self):
        pass
    
    def run(self):
        self.fail = True
        pass

class FakeTestSuccessful(Base.BaseTestcase):
    
    def __init__(self):
        Base.BaseTestcase.__init__(self, "FakeTestSuccessful", "This is just a fake test to prove the report modules.")
        hosts = data.getHosts()
        
        """ pick two host out of the list """
        self.hosts.append(hosts[0])
        self.hosts.append(hosts[1])
        
        self.defineGraph(1, "Throughput", "Throughput of the file transfers", "X Axis", "Y Axis")
        self.defineGraph(2, "Persistent Throughput", "Throughput of the file transfers", persistent=True)
    
    def prepare(self):
        pass
    
    def run(self):
        self.addGraphableData(1, 1, 2)
        
        self.printReport("Some output")
        
        self.addGraphableData(1, 2, 3)
        self.addGraphableData(1, 3, 4)
        self.addGraphableData(1, 4, 1)
        
        self.printReport("Hello World, seconds line")

        self.printReport("Hello World")
        
        self.addGraphableData(2, 2, 3)
        self.addGraphableData(2, 3, 4)
        self.addGraphableData(2, 4, 1)
        
        pass