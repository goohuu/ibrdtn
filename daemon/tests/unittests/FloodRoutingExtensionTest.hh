/* $Id: templateengine.py 2241 2006-05-22 07:58:58Z fischer $ */

///
/// @file        FloodRoutingExtensionTest.hh
/// @brief       CPPUnit-Tests for class FloodRoutingExtension
/// @author      Author Name (email@mail.address)
/// @date        Created at 2010-11-01
/// 
/// @version     $Revision: 2241 $
/// @note        Last modification: $Date: 2006-05-22 09:58:58 +0200 (Mon, 22 May 2006) $
///              by $Author: fischer $
///

 
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifndef FLOODROUTINGEXTENSIONTEST_HH
#define FLOODROUTINGEXTENSIONTEST_HH
class FloodRoutingExtensionTest : public CppUnit::TestFixture {
	private:
	public:
		/*=== BEGIN tests for class 'FloodRoutingExtension' ===*/
		void testNotify();
		void testRun();
		void test__cancellation();
		/*=== END   tests for class 'FloodRoutingExtension' ===*/

		void setUp();
		void tearDown();


		CPPUNIT_TEST_SUITE(FloodRoutingExtensionTest);
			CPPUNIT_TEST(testNotify);
			CPPUNIT_TEST(testRun);
			CPPUNIT_TEST(test__cancellation);
		CPPUNIT_TEST_SUITE_END();
};
#endif /* FLOODROUTINGEXTENSIONTEST_HH */
