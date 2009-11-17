/* 
 * File:   DiscoveryServiceProvider.h
 * Author: morgenro
 *
 * Created on 17. November 2009, 17:27
 */

#ifndef _DISCOVERYSERVICEPROVIDER_H
#define	_DISCOVERYSERVICEPROVIDER_H

#include "ibrdtn/default.h"

namespace dtn
{
    namespace net
    {
        class DiscoveryServiceProvider
        {
        public:
            virtual void update(std::string &name, std::string &data) = 0;
        };
    }
}

#endif	/* _DISCOVERYSERVICEPROVIDER_H */

