/*
 * File:   NetInterface.h
 * Author: morgenro
 *
 * Created on 12. November 2009, 14:57
 */

#ifndef _NETINTERFACE_H
#define	_NETINTERFACE_H

#include <ibrdtn/default.h>
#include <netinet/in.h>

namespace dtn
{
    namespace net
    {
        class NetInterface
        {
        public:
            enum NetworkType
            {
                NETWORK_UNKNOWN = 0,
                NETWORK_TCP = 1,
                NETWORK_UDP = 2
            };

            NetInterface(NetworkType type, string name, string systemname, unsigned int port = 4556, unsigned int mtu = 0);
            NetInterface(NetworkType type, string name, string address, string broadcastaddress = "255.255.255.255", unsigned int port = 4556, unsigned int mtu = 1280);
            NetInterface(const NetInterface& orig);
            virtual ~NetInterface();

            NetworkType getType() const;

            string getName() const;
            string getSystemName() const;
            string getAddress() const;
            string getBroadcastAddress() const;
            unsigned int getPort() const;
            unsigned int getMTU() const;

            void getInterfaceAddress(struct in_addr *ret) const;
            void getInterfaceBroadcastAddress(struct in_addr *ret) const;

        private:
            static void getInterfaceAddress(string interface, struct in_addr *ret);
            static void getInterfaceBroadcastAddress(string interface, struct in_addr *ret);

            static string getInterfaceAddress(string interface);
            static string getInterfaceBroadcastAddress(string interface);

            NetworkType _type;
            string _name;
            string _systemname;
            string _address;
            string _broadcastaddress;
            unsigned int _port;
            unsigned int _mtu;
        };
    }
}

#endif	/* _NETINTERFACE_H */

