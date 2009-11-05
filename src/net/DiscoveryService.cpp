/*
 * DiscoveryService.cpp
 *
 *  Created on: 11.09.2009
 *      Author: morgenro
 */

#include "net/DiscoveryService.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BundleString.h"
#include <string>

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		DiscoveryService::DiscoveryService()
		{
		}

		DiscoveryService::DiscoveryService(std::string name, std::string parameters)
		 : _service_name(name), _service_parameters(parameters)
		{
		}

		DiscoveryService::~DiscoveryService()
		{
		}

		size_t DiscoveryService::getLength() const
		{
			return 1;
		}

		std::string DiscoveryService::getName() const
		{
			return _service_name;
		}

		std::string DiscoveryService::getParameters() const
		{
			return _service_parameters;
		}

		std::ostream &operator<<(std::ostream &stream, const DiscoveryService &service)
		{
			BundleString name(service._service_name);
			BundleString parameters(service._service_parameters);

			stream << name << parameters;

			return stream;
		}

		std::istream &operator>>(std::istream &stream, DiscoveryService &service)
		{
			BundleString name;
			BundleString parameters;

			stream >> name >> parameters;

			service._service_name = (std::string)name;
			service._service_parameters = (std::string)parameters;

			return stream;
		}
	}
}
