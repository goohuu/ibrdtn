/* 
 * File:   DiscoveryServiceProvider.h
 * Author: morgenro
 *
 * Created on 17. November 2009, 17:27
 */

#ifndef _DISCOVERYSERVICEPROVIDER_H
#define	_DISCOVERYSERVICEPROVIDER_H

#include <ibrcommon/net/NetInterface.h>
#include <string>

namespace dtn
{
	namespace net
	{
		class DiscoveryServiceProvider
		{
		public:
			/**
			 * Updates an discovery service block with current values
			 * @param name
			 * @param data
			 */
			virtual void update(std::string &name, std::string &data) = 0;

			virtual bool onInterface(const ibrcommon::NetInterface&) const
			{
				return true;
			}
		};
	}
}

#endif	/* _DISCOVERYSERVICEPROVIDER_H */

