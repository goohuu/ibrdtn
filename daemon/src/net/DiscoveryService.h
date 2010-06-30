/*
 * DiscoveryService.h
 *
 *  Created on: 11.09.2009
 *      Author: morgenro
 */

#ifndef DISCOVERYSERVICE_H_
#define DISCOVERYSERVICE_H_

#include <net/DiscoveryServiceProvider.h>
#include <ibrcommon/net/NetInterface.h>
#include <stdlib.h>
#include <iostream>

namespace dtn
{
	namespace net
	{
		class DiscoveryService
		{
		public:
			DiscoveryService();
			DiscoveryService(DiscoveryServiceProvider *provider);
			DiscoveryService(std::string name, std::string parameters);
			virtual ~DiscoveryService();

			size_t getLength() const;

			std::string getName() const;
			std::string getParameters() const;

			/**
			 * updates the values of the service block
			 */
			void update();

			bool onInterface(const ibrcommon::NetInterface &net) const;

		protected:
			std::string _service_name;
			std::string _service_parameters;
			DiscoveryServiceProvider *_provider;

		private:
			friend std::ostream &operator<<(std::ostream &stream, const DiscoveryService &service);
			friend std::istream &operator>>(std::istream &stream, DiscoveryService &service);
		};
	}
}

#endif /* DISCOVERYSERVICE_H_ */
