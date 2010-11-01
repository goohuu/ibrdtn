/* $Id: templateengine.py 2241 2006-05-22 07:58:58Z fischer $ */

///
/// @file        ComponentTest.hh
/// @brief       CPPUnit-Tests for class Component
/// @author      Author Name (email@mail.address)
/// @date        Created at 2010-11-01
/// 
/// @version     $Revision: 2241 $
/// @note        Last modification: $Date: 2006-05-22 09:58:58 +0200 (Mon, 22 May 2006) $
///              by $Author: fischer $
///

 
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifndef COMPONENTTEST_HH
#define COMPONENTTEST_HH
class ComponentTest : public CppUnit::TestFixture {
	private:
	public:
		/*=== BEGIN tests for class 'IndependentComponent' ===*/
		void testRun();
		/*=== END   tests for class 'IndependentComponent' ===*/

		void setUp();
		void tearDown();


		CPPUNIT_TEST_SUITE(ComponentTest);
			CPPUNIT_TEST(testRun);
		CPPUNIT_TEST_SUITE_END();
};
#endif /* COMPONENTTEST_HH */
