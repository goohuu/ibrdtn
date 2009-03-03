/*
 * DiscoveryBlockTestSuite.cpp
 *
 *  Created on: 27.02.2009
 *      Author: morgenro
 */

#include "testsuite/DiscoveryBlockTestSuite.h"

#ifdef USE_EMMA_CODE

#include "testsuite/SelfTestSuite.h"
#include "testsuite/TestUtils.h"
#include "data/Bundle.h"
#include "data/BundleFactory.h"
#include "emma/DiscoverBlock.h"
#include <iostream>
#include "utils/Utils.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>

using namespace dtn::data;
using namespace emma;

namespace dtn
{
namespace testsuite
{
	DiscoveryBlockTestSuite::DiscoveryBlockTestSuite()
	{
		BundleFactory &fac = BundleFactory::getInstance();
		m_dfac = new DiscoverBlockFactory();
		fac.registerExtensionBlock(m_dfac);
	}

	DiscoveryBlockTestSuite::~DiscoveryBlockTestSuite()
	{
		BundleFactory &fac = BundleFactory::getInstance();
		fac.unregisterExtensionBlock(m_dfac);
		delete m_dfac;
	}

	bool DiscoveryBlockTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "DiscoveryBlockTestSuite... ";

		if ( !createTest() )
		{
			cout << endl << "createTest failed" << endl;
			ret = false;
		}

		if ( !parseTest() )
		{
			cout << endl << "parseTest failed" << endl;
			ret = false;
		}

		if ( !coordTest() )
		{
			cout << endl << "coordTest failed" << endl;
			ret = false;
		}

		if (ret) cout << "\t\tpassed" << endl;

		return ret;
	}

	bool DiscoveryBlockTestSuite::coordTest()
	{
		BundleFactory &fac = BundleFactory::getInstance();
		dtn::data::Bundle *out = fac.newBundle();

		DiscoverBlock *discover = DiscoverBlockFactory::newDiscoverBlock();
		out->appendBlock(discover);

		discover->setConnectionAddress("1.2.3.4");
		discover->setConnectionPort(1234);
		discover->setLatitude(10.0);
		discover->setLongitude(20.0);

		unsigned char *data = out->getData();
		unsigned int length = out->getLength();

		delete out;

		out = fac.parse(data, length);

		free(data);

		list<Block*> blocks = out->getBlocks(DiscoverBlock::BLOCK_TYPE);

		if ( blocks.empty() )
		{
			delete out;
			cout << "no discovery block available!" << endl;
			return false;
		}

		discover = dynamic_cast<DiscoverBlock*>(blocks.front());

		if ( discover == NULL )
		{
			cout << "the block isn't a discovery block." << endl;
			delete out;
			return false;
		}

		if (discover->getConnectionAddress() != "1.2.3.4")
		{
			cout << "wrong connection address" << endl;
			delete out;
			return false;
		}

		if (discover->getConnectionPort() != 1234)
		{
			cout << "wrong connection port" << endl;
			delete out;
			return false;
		}

		if (discover->getLatitude() != 10.0)
		{
			cout << "wrong latitude" << endl;
			delete out;
			return false;
		}

		if (discover->getLongitude() != 20.0)
		{
			cout << "wrong longitude" << endl;
			delete out;
			return false;
		}

		delete out;

		return true;
	}

	bool DiscoveryBlockTestSuite::parseTest()
	{
		BundleFactory &fac = BundleFactory::getInstance();
		dtn::data::Bundle *out = fac.newBundle();

		DiscoverBlock *discover = DiscoverBlockFactory::newDiscoverBlock();
		out->appendBlock(discover);

		discover->setConnectionAddress("1.2.3.4");
		discover->setConnectionPort(1234);

		unsigned char *data = out->getData();
		unsigned int length = out->getLength();

		delete out;

		out = fac.parse(data, length);

		free(data);

		list<Block*> blocks = out->getBlocks(DiscoverBlock::BLOCK_TYPE);

		if ( blocks.empty() )
		{
			delete out;
			cout << "no discovery block available!" << endl;
			return false;
		}

		discover = dynamic_cast<DiscoverBlock*>(blocks.front());

		if ( discover == NULL )
		{
			cout << "the block isn't a discovery block." << endl;
			delete out;
			return false;
		}

		if (discover->getConnectionAddress() != "1.2.3.4")
		{
			cout << "wrong connection address" << endl;
			delete out;
			return false;
		}

		if (discover->getConnectionPort() != 1234)
		{
			cout << "wrong connection port" << endl;
			delete out;
			return false;
		}

		delete out;

		return true;
	}

	bool DiscoveryBlockTestSuite::createTest()
	{
		DiscoverBlock *discover = DiscoverBlockFactory::newDiscoverBlock();

		if ( discover->getType() != DiscoverBlock::BLOCK_TYPE )
		{
			delete discover;
			return false;
		}

		delete discover;
		return true;
	}
}
}

#endif
