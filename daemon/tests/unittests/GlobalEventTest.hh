/* $Id: templateengine.py 2241 2006-05-22 07:58:58Z fischer $ */

///
/// @file        GlobalEventTest.hh
/// @brief       CPPUnit-Tests for class GlobalEvent
/// @author      Author Name (email@mail.address)
/// @date        Created at 2010-11-01
/// 
/// @version     $Revision: 2241 $
/// @note        Last modification: $Date: 2006-05-22 09:58:58 +0200 (Mon, 22 May 2006) $
///              by $Author: fischer $
///

 
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifndef GLOBALEVENTTEST_HH
#define GLOBALEVENTTEST_HH
class GlobalEventTest : public CppUnit::TestFixture {
	private:
	public:
		/*=== BEGIN tests for class 'GlobalEvent' ===*/
		void testGetName();
		void testGetAction();
		void testRaise();
		void testToString();
		/*=== END   tests for class 'GlobalEvent' ===*/

		void setUp();
		void tearDown();


		CPPUNIT_TEST_SUITE(GlobalEventTest);
			CPPUNIT_TEST(testGetName);
			CPPUNIT_TEST(testGetAction);
			CPPUNIT_TEST(testRaise);
			CPPUNIT_TEST(testToString);
		CPPUNIT_TEST_SUITE_END();
};
#endif /* GLOBALEVENTTEST_HH */
