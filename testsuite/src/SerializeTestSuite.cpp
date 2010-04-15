/*
 * SerializeTestSuite.cpp
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#include "testsuite/SerializeTestSuite.h"
#include "net/DiscoveryAnnouncement.h"
#include "ibrdtn/data/BundleString.h"
#include "net/DiscoveryService.h"
#include "ibrdtn/data/SDNV.h"
#include <iostream>

namespace dtn
{
	namespace testsuite
	{
		SerializeTestSuite::SerializeTestSuite()
		{

		}

		SerializeTestSuite::~SerializeTestSuite()
		{

		}

		bool SerializeTestSuite::BundleString()
		{
			stringstream ss;

			dtn::data::BundleString bstring1("Hallo Welt!");
			dtn::data::BundleString bstring2("Ich bin da!");

			ss << bstring1 << bstring2;
			ss >> bstring1 >> bstring2;

			if (bstring1 != "Hallo Welt!") return false;
			if (bstring2 != "Ich bin da!") return false;

			return true;
		}

		bool SerializeTestSuite::DiscoveryAnnouncement()
		{
			stringstream ss;

			dtn::net::DiscoveryAnnouncement announce(false, dtn::data::EID("dtn:testnode1"));
			dtn::net::DiscoveryService service("Hello", "World");
			announce.addService(service);

			ss << announce;
			ss >> announce;

			if (announce.getEID() != dtn::data::EID("dtn:testnode1")) return false;

			return true;
		}

		bool SerializeTestSuite::SDNV()
		{
			stringstream ss;

			dtn::data::SDNV value1(42);
			dtn::data::SDNV value2(23);

			ss << value1 << value2;
			ss >> value1 >> value2;

			if (value1 != 42) return false;
			if (value2 != 23) return false;

			return true;
		}

		bool SerializeTestSuite::DiscoveryService()
		{
			stringstream ss;

			dtn::net::DiscoveryService service("Hello", "World");

			ss << service;
			ss >> service;

			if (service.getName() != "Hello") return false;
			if (service.getParameters() != "World") return false;

			return true;
		}

		bool SerializeTestSuite::runAllTests()
		{
			bool ret = true;
			cout << "# SerializeTestSuite #" << endl;

			cout << "Test SDNV... ";
			if ( !SDNV() )
			{
				cout << "failed";
				ret = false;
			} else	cout << "success";
			cout << endl;

			cout << "Test BundleString... ";
			if ( !BundleString() )
			{
				cout << "failed";
				ret = false;
			} else	cout << "success";
			cout << endl;

			cout << "Test DiscoveryService... ";
			if ( !DiscoveryService() )
			{
				cout << "() failed";
				ret = false;
			} else	cout << "success";
			cout << endl;

			cout << "Test DiscoveryAnnouncement... ";
			if ( !DiscoveryAnnouncement() )
			{
				cout << "failed";
				ret = false;
			} else	cout << "success";
			cout << endl;

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}
	}
}

