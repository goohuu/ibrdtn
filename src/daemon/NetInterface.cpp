/* 
 * File:   NetInterface.cpp
 * Author: morgenro
 * 
 * Created on 12. November 2009, 14:57
 */

#include "daemon/NetInterface.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>

#include <unistd.h>

namespace dtn
{
    namespace daemon
    {
        NetInterface::NetInterface(NetInterface::NetworkType type, string name, string systemname, unsigned int port, unsigned int mtu)
         : _type(type), _name(name), _systemname(systemname), _port(port), _mtu(mtu)
        {
            _address = getInterfaceAddress(systemname);
            _broadcastaddress = getInterfaceBroadcastAddress(systemname);
        }

        NetInterface::NetInterface(NetInterface::NetworkType type, string name, string address, string broadcastaddress, unsigned int port, unsigned int mtu)
         : _type(type), _name(name), _systemname("static"), _address(address), _broadcastaddress(broadcastaddress), _port(port), _mtu(mtu)
        {
        }

        NetInterface::NetInterface(const NetInterface& orig)
        : _type(orig._type), _name(orig._name), _systemname(orig._systemname), _address(orig._address), _broadcastaddress(orig._broadcastaddress), _port(orig._port), _mtu(orig._mtu)
        {

        }

        NetInterface::~NetInterface()
        {

        }

        NetInterface::NetworkType NetInterface::getType() const
        {
            return _type;
        }

        string NetInterface::getName() const
        {
            return _name;
        }

        string NetInterface::getSystemName() const
        {
            return _systemname;
        }

        string NetInterface::getAddress() const
        {
            return _address;
        }

        string NetInterface::getBroadcastAddress() const
        {
            return _broadcastaddress;
        }

        unsigned int NetInterface::getPort() const
        {
            return _port;
        }

        unsigned int NetInterface::getMTU() const
        {
            return _mtu;
        }

        string NetInterface::getInterfaceAddress(string interface)
        {
                // define the return value
                string ret = "0.0.0.0";

                struct ifaddrs *ifap = NULL;
                int status = getifaddrs(&ifap);

                if ((status != 0) || (ifap == NULL))
                {
                        // error, return with default address
                        return ret;
                }

                for (struct ifaddrs *iter = ifap; iter != NULL; iter = iter->ifa_next)
                {
                        if (iter->ifa_addr == NULL) continue;
                        if ((iter->ifa_flags & IFF_UP) == 0) continue;

                        sockaddr *addr = iter->ifa_addr;

                        // currently only IPv4 is supported
                        if (addr->sa_family != AF_INET) continue;

                        // check the name of the interface
                        if (string(iter->ifa_name) != interface) continue;

                        sockaddr_in *ip = (sockaddr_in*)iter->ifa_addr;

                        ret = string(inet_ntoa(ip->sin_addr));
                        break;
                }

                freeifaddrs(ifap);

                return ret;
        }

        string NetInterface::getInterfaceBroadcastAddress(string interface)
        {
                // define the return value
                string ret = "255.255.255.255";

                struct ifaddrs *ifap = NULL;
                int status = getifaddrs(&ifap);

                if ((status != 0) || (ifap == NULL))
                {
                        // error, return with default address
                        return ret;
                }

                for (struct ifaddrs *iter = ifap; iter != NULL; iter = iter->ifa_next)
                {
                        if (iter->ifa_addr == NULL) continue;
                        if ((iter->ifa_flags & IFF_UP) == 0) continue;

                        sockaddr *addr = iter->ifa_addr;

                        // currently only IPv4 is supported
                        if (addr->sa_family != AF_INET) continue;

                        // check the name of the interface
                        if (string(iter->ifa_name) != interface) continue;

                        if (iter->ifa_flags & IFF_BROADCAST)
                        {
                                sockaddr_in *broadcast = (sockaddr_in*)iter->ifa_ifu.ifu_broadaddr;
                                ret = string(inet_ntoa(broadcast->sin_addr));
                                break;
                        }
                }

                freeifaddrs(ifap);

                return ret;
        }
    }
}
