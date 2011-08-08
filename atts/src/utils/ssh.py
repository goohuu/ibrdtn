'''
Created on 29.03.2010

@author: morgenro
'''

import os
import traceback
import paramiko
import threading
import socket

class RemoteHost(object):
    '''
    classdocs
    '''
    
    class IgnorePolicy (paramiko.MissingHostKeyPolicy):
        """
        Policy for logging a python-style warning for an unknown host key, but
        accepting it. This is used by L{SSHClient}.
        """
        def missing_host_key(self, client, hostname, key):
            pass
        
    class State:
        unknown=0
        CLOSED=1
        OPEN=2

    def __init__(self, address, username, password=None, keyfile=None):
        '''
        Constructor
        '''
        self.address = address
        self.username = username
        self.password = password
        self.key_filename=keyfile
        self.state=RemoteHost.State.CLOSED

        self.executeLock=threading.Lock()
        
        os.system("ssh-keyscan -t rsa,dsa " + self.address + " >> ~/.ssh/known_hosts")
        
        #keys = paramiko.util.load_host_keys(os.path.expanduser('~/.ssh/known_hosts'))
        
    def connect(self):
        print u"connecting to " + self.address
        
        # now, connect and use paramiko Client to negotiate SSH2 across the connection
        try:
            self.client = paramiko.SSHClient()
            #self.client.load_system_host_keys()
            policy = RemoteHost.IgnorePolicy()
            self.client.set_missing_host_key_policy(policy)
            self.client.connect(self.address, 22, username=self.username, password=self.password, pkey=None, key_filename=self.key_filename, timeout=None, allow_agent=False, look_for_keys=False)
            self.state=RemoteHost.State.OPEN
            
            # get the SSH transport
            self.transport = self.client.get_transport()

        except Exception, e:
            print '*** Caught exception: %s: %s' % (e.__class__, e)
            traceback.print_exc()
            self.close()
        
    def execute(self, command):
        self.executeLock.acquire()
        try:
            if self.state != RemoteHost.State.CLOSED:
                print("run command: " + command)
                s = self.transport.open_session()
                s.exec_command(command)
                s.recv_exit_status()
        except socket.error:
            print("can not execute ssh command")
            pass
        self.executeLock.release()
    
    def isConnected(self):
        return (self.state != RemoteHost.State.CLOSED)
    
    def put(self, local, remote):
        #sftpc = self.client.open_sftp()
        #sftpc.put(local, remote)
        #sftpc.close()
        os.system("scp -i " + self.key_filename + " " + local + " root@" + self.address + ":" + remote)
    
    def get(self, remote, local):
        #sftpc = self.client.open_sftp()
        #sftpc.get(remote, local)
        #sftpc.close()
        os.system("scp -i " + self.key_filename + " root@" + self.address + ":" + remote + " " + local)
    
    def close(self):
        if self.state != RemoteHost.State.CLOSED:
            try:
                self.transport.close()
                self.state=RemoteHost.State.CLOSED
            except:
                pass
            
            print u"connection to " + self.address + " closed"
