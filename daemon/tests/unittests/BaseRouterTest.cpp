/* $Id: templateengine.py 2241 2006-05-22 07:58:58Z fischer $ */

///
/// @file        BaseRouterTest.cpp
/// @brief       CPPUnit-Tests for class BaseRouter
/// @author      Author Name (email@mail.address)
/// @date        Created at 2010-11-01
/// 
/// @version     $Revision: 2241 $
/// @note        Last modification: $Date: 2006-05-22 09:58:58 +0200 (Mon, 22 May 2006) $
///              by $Author: fischer $
///

 

#include "BaseRouterTest.hh"
#include "src/routing/BaseRouter.h"
#include "src/core/BundleStorage.h"
#include "src/core/Node.h"
#include "tests/tools/EventSwitchLoop.h"
#include "src/net/BundleReceivedEvent.h"
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/EID.h>
#include <ibrcommon/thread/Thread.h>


CPPUNIT_TEST_SUITE_REGISTRATION(BaseRouterTest);

/*========================== tests below ==========================*/

/*=== BEGIN tests for class 'BaseRouter' ===*/
/*=== BEGIN tests for class 'Extension' ===*/
void BaseRouterTest::testGetRouter()
{
	class ExtensionTest : public dtn::routing::BaseRouter::Extension
	{
	public:
		ExtensionTest() {};
		~ExtensionTest() {};

		void notify(const dtn::core::Event*) {};

		dtn::routing::BaseRouter* testGetRouter()
		{
			return getRouter();
		};
	};

	dtn::routing::BaseRouter router(_storage);
	ExtensionTest e;
	CPPUNIT_ASSERT_EQUAL(&router, e.testGetRouter());
}

/*=== END   tests for class 'Extension' ===*/

void BaseRouterTest::testAddExtension()
{
	/* test signature (BaseRouter::Extension *extension) */
	class ExtensionTest : public dtn::routing::BaseRouter::Extension
	{
	public:
		ExtensionTest() {};
		~ExtensionTest() {};

		void notify(const dtn::core::Event*) {};

		dtn::routing::BaseRouter* testGetRouter()
		{
			return getRouter();
		};
	};

	dtn::routing::BaseRouter router(_storage);
	ExtensionTest e;
	router.addExtension(&e);
}

void BaseRouterTest::testTransferTo()
{
	/* test signature (const dtn::data::EID &destination, const dtn::data::BundleID &id) */
	dtn::data::EID eid("dtn://no-neighbor");
	dtn::data::Bundle b;
	dtn::routing::BaseRouter router(_storage);
	router.transferTo(eid, b);
}

void BaseRouterTest::testRaiseEvent()
{
	/* test signature (const dtn::core::Event *evt) */
	dtn::routing::BaseRouter router(_storage);
	dtn::data::EID eid("dtn://no-neighbor");
	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://testcase-one/foo");

	CPPUNIT_ASSERT_EQUAL(false, router.isKnown(b));

	router.initialize();
	{
		ibrtest::EventSwitchLoop esl; esl.start();

		// send a bundle
		dtn::net::BundleReceivedEvent::raise(eid, b);
	}
	router.terminate();

	// this bundle has to be known in future
	CPPUNIT_ASSERT_EQUAL(true, router.isKnown(b));
}

void BaseRouterTest::testGetBundle()
{
	/* test signature (const dtn::data::BundleID &id) */
	dtn::routing::BaseRouter router(_storage);

	dtn::data::Bundle b1;
	_storage.store(b1);

	try {
		dtn::data::Bundle b2 = router.getBundle(b1);
	} catch (const ibrcommon::Exception&) {
		CPPUNIT_FAIL("no bundle returned");
	}
}

void BaseRouterTest::testGetStorage()
{
	/* test signature () */
	dtn::routing::BaseRouter router(_storage);
	CPPUNIT_ASSERT_EQUAL((dtn::core::BundleStorage*)&_storage, &router.getStorage());
}

void BaseRouterTest::testIsKnown()
{
	/* test signature (const dtn::data::BundleID &id) */
	dtn::routing::BaseRouter router(_storage);

	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://testcase-one/foo");

	CPPUNIT_ASSERT_EQUAL(false, router.isKnown(b));

	router.setKnown(b);

	CPPUNIT_ASSERT_EQUAL(true, router.isKnown(b));
}

void BaseRouterTest::testSetKnown()
{
	/* test signature (const dtn::data::MetaBundle &meta) */
	dtn::routing::BaseRouter router(_storage);

	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://testcase-one/foo");

	router.setKnown(b);
	CPPUNIT_ASSERT_EQUAL(true, router.isKnown(b));
}

void BaseRouterTest::testGetSummaryVector()
{
	/* test signature () */
	dtn::routing::BaseRouter router(_storage);

	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://testcase-one/foo");

	CPPUNIT_ASSERT_EQUAL(false, router.isKnown(b));

	router.setKnown(b);

	CPPUNIT_ASSERT_EQUAL(true, router.getSummaryVector().contains(b));
}

/*=== END   tests for class 'BaseRouter' ===*/

void BaseRouterTest::setUp()
{
	_storage.clear();
}

void BaseRouterTest::tearDown()
{
}

