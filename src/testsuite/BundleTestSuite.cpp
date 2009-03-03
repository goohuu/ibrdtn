#include "testsuite/BundleTestSuite.h"
#include "testsuite/SelfTestSuite.h"
#include "data/Bundle.h"
#include "data/BundleFactory.h"
#include "testsuite/TestUtils.h"
#include "data/PayloadBlockFactory.h"
#include "data/CustodySignalBlock.h"
#include <iostream>
#include <list>
#include "utils/Utils.h"
#include <limits.h>
#include <cstdlib>
#include "data/BlockFactory.h"

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	BundleTestSuite::BundleTestSuite()
	{
	}

	BundleTestSuite::~BundleTestSuite()
	{
	}

	bool BundleTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "BundleTestSuite... ";

		if ( !createTest() )
		{
			cout << endl << "createTest failed" << endl;
			ret = false;
		}

		if ( !compareTest() )
		{
			cout << endl << "compareTest failed" << endl;
			ret = false;
		}

		if ( !cutTest() )
		{
			cout << endl << "cutTest failed" << endl;
			ret = false;
		}

		if ( !mergeTest() )
		{
			cout << endl << "mergeTest failed" << endl;
			ret = false;
		}

		if ( !sliceTest() )
		{
			cout << endl << "cutTest failed" << endl;
			ret = false;
		}

		if ( !splitTest() )
		{
			cout << endl << "splitTest failed" << endl;
			ret = false;
		}

		if (ret) cout << "\t\t\tpassed" << endl;

		return ret;
	}

	bool BundleTestSuite::cutTest()
	{
		bool ret = true;

		Bundle *bundle = TestUtils::createTestBundle(1024);
		BundleFactory &fac = BundleFactory::getInstance();

		unsigned int offset = 0;
		Bundle *first = fac.cut(bundle, 512, offset);
		Bundle *second = fac.cut(bundle, UINT_MAX, offset);

		PayloadBlock *p1 = first->getPayloadBlock();
		PayloadBlock *p2 = second->getPayloadBlock();

		if ( p1->getLength() != 512 ) ret = false;
		if ( p2->getLength() != 512 ) ret = false;

		if ( first->getSource() != bundle->getSource() ) ret = false;
		if ( first->getDestination() != bundle->getDestination() ) ret = false;
		if ( first->getCustodian() != bundle->getCustodian() ) ret = false;
		if ( first->getReportTo() != bundle->getReportTo() ) ret = false;

		for (size_t i = CREATION_TIMESTAMP; i <= DICTIONARY_LENGTH; i++)
		{
			if ( first->getInteger(BUNDLE_FIELDS(i)) != bundle->getInteger(BUNDLE_FIELDS(i)) )
			{
				ret = false;
				break;
			}
		}

		if ( second->getSource() != bundle->getSource() ) ret = false;
		if ( second->getDestination() != bundle->getDestination() ) ret = false;
		if ( second->getCustodian() != bundle->getCustodian() ) ret = false;
		if ( second->getReportTo() != bundle->getReportTo() ) ret = false;

		for (size_t i = CREATION_TIMESTAMP; i <= DICTIONARY_LENGTH; i++)
		{
			if ( first->getInteger(BUNDLE_FIELDS(i)) != bundle->getInteger(BUNDLE_FIELDS(i)) )
			{
				ret = false;
				break;
			}
		}

		// check fragmentation parameters
		if (first->getInteger(FRAGMENTATION_OFFSET) != 0) ret = false;
		if (second->getInteger(FRAGMENTATION_OFFSET) != 512) ret = false;

		if (first->getInteger(APPLICATION_DATA_LENGTH) != 1024) ret = false;
		if (second->getInteger(APPLICATION_DATA_LENGTH) != 1024) ret = false;

		delete first;
		delete second;
		delete bundle;

		return ret;
	}

	bool BundleTestSuite::sliceTest()
	{
		bool ret = true;

		// Erstelle ein "großes" Bundle
		Bundle *bundle = TestUtils::createTestBundle(1024);

		list<Bundle*> bundles;

		unsigned int offset = 0;
		Bundle *fragment = BundleFactory::slice(bundle, 200, offset);

		while (fragment != NULL)
		{
			bundles.push_back(fragment);
			fragment = BundleFactory::slice(bundle, 200, offset);
		}

		// Size test
		unsigned int payload_sum = 0;
		list<Bundle*>::iterator iter = bundles.begin();
		while (iter != bundles.end())
		{
			payload_sum += (*iter)->getPayloadBlock()->getLength();
			iter++;
		}

		if (payload_sum != 1024)
		{
			cout << "Bundle nicht korrekt aufgeteilt!" << endl;
			ret = false;
		}

		if (bundles.size() != 7)
		{
			cout << "Bundle nicht korrekt aufgeteilt!" << endl;
			ret = false;
		}

		// Merge
		Bundle *merged = BundleFactory::getInstance().merge( bundles );

		// delete the fragments
		iter = bundles.begin();
		while (iter != bundles.end())
		{
			delete (*iter);
			iter++;
		}

		if (!TestUtils::compareBundles(bundle, merged))
		{
			ret = false;
		}

		delete merged;
		delete bundle;
		return ret;
	}

	bool BundleTestSuite::mergeTest()
	{
		bool ret = true;

		// Erstelle ein "großes" Bundle
		Bundle *bundle = TestUtils::createTestBundle(1024);
		BundleFactory &fac = BundleFactory::getInstance();

		unsigned int offset = 0;
		Bundle *first = fac.cut(bundle, 512, offset);
		Bundle *second = fac.cut(bundle, UINT_MAX, offset);

		// Merge
		Bundle *merge = fac.merge( first, second );

		delete first;
		delete second;

		if ( merge->getPrimaryFlags().isFragment())
		{
			ret = false;
			cout << "bundle is still a fragment!" << endl;
		}

		if (!TestUtils::compareBundles(bundle, merge))
		{
			ret = false;
		}

		if (merge != NULL) delete merge;

		// merge test, the second
		offset = 0;
		first = fac.cut(bundle, 512, offset);
		second = fac.cut(bundle, 128, offset);
		Bundle *third = fac.cut(bundle, 128, offset);
		Bundle *fourth = fac.cut(bundle, UINT_MAX, offset);

		// first merge should give a new fragment
		merge = fac.merge( first, second );

		if ( !merge->getPrimaryFlags().isFragment())
		{
			ret = false;
			cout << "bundle isn't a fragment!" << endl;
		}

		// second merge should give the hole bundle
		Bundle *tmp = merge;
		merge = fac.merge( tmp, third );
		delete tmp;

		tmp = merge;
		merge = fac.merge( tmp, fourth );
		delete tmp;

		if ( merge->getPrimaryFlags().isFragment())
		{
			ret = false;
			cout << "bundle is still a fragment!" << endl;
		}

		delete first;
		delete second;
		delete third;
		delete fourth;

		if (!TestUtils::compareBundles(bundle, merge))
		{
			ret = false;
		}

		if (merge != NULL) delete merge;

		delete bundle;

		return ret;
	}

	bool BundleTestSuite::splitTest()
	{
		bool ret = true;

		// Erstelle ein "großes" Bundle
		Bundle *bundle = TestUtils::createTestBundle(1024);

		BundleFactory &fac = BundleFactory::getInstance();

		list<Bundle*> bundles = fac.split(bundle, 200);

		if (bundles.size() != 7)
		{
			cout << "Bundle nicht korrekt aufgeteilt!" << endl;
			ret = false;
		}

		// Merge
		Bundle *merged = fac.merge( bundles );

		// delete the fragments
		list<Bundle*>::const_iterator iter = bundles.begin();
		while (iter != bundles.end())
		{
			delete (*iter);
			iter++;
		}

		if (merged == NULL)
		{
			return false;
		}

		if (!TestUtils::compareBundles(bundle, merged))
		{
			ret = false;
		}

		delete merged;
		delete bundle;

		return ret;
	}

	bool BundleTestSuite::compareTest()
	{
		bool ret = true;

		// create a test bundle
		Bundle *out = TestUtils::createTestBundle();

		BundleFactory &fac = BundleFactory::getInstance();

		// copy the test bundle
		unsigned char *data = out->getData();
		Bundle *in = fac.parse( data, out->getLength() );
		free(data);

		if (*in != *out)
		{
			testsuite::TestUtils::debugData(in->getData(), in->getLength());
			testsuite::TestUtils::debugData(out->getData(), out->getLength());
			ret = false;
		}

		delete in;
		delete out;

		return ret;
	}

	bool BundleTestSuite::createTest()
	{
		BundleFactory &fac = BundleFactory::getInstance();

		// Erstellt ein Testbundle
		Bundle *out = TestUtils::createTestBundle();

		// Setze den Empfänger und den Absender ein
		out->setDestination( "dtn://wks1" );
		out->setSource( "dtn:local" );
		out->setInteger( LIFETIME, 20 );

		try {
			unsigned char *data = out->getData();
			unsigned int length = out->getLength();

			Bundle *in = fac.parse( data, length );
			free(data);

			if (in->getLength() != length)
			{
				cout << "didn't parsed the hole bundle" << endl;
				delete in;
				delete out;
				return false;
			}

			// Prüfe Bundle
			if ( in->getDestination() != "dtn://wks1")
			{
				cout << "Destination != dtn://wks1, Value: " << in->getDestination() << endl;
				delete in;
				delete out;
				return false;
			}

			// Prüfe den nächsten Block
			if ( in->getPayloadBlock() == NULL )
			{
				cout << "Payloadblock missing" << endl;
				delete in;
				delete out;
				return false;
			}

			delete in;
			delete out;

			return true;

		} catch (Exception ex) {
			cout << "Fehler beim decodieren des Bundle: " << ex.what() << endl;
			delete out;
			return false;
		}


	}
}
}
