/* $Id: templateengine.py 2241 2006-05-22 07:58:58Z fischer $ */

///
/// @file        DebuggerTest.hh
/// @brief       CPPUnit-Tests for class Debugger
/// @author      Author Name (email@mail.address)
/// @date        Created at 2010-11-01
/// 
/// @version     $Revision: 2241 $
/// @note        Last modification: $Date: 2006-05-22 09:58:58 +0200 (Mon, 22 May 2006) $
///              by $Author: fischer $
///

 
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifndef DEBUGGERTEST_HH
#define DEBUGGERTEST_HH
class DebuggerTest : public CppUnit::TestFixture {
	private:
	public:
		/*=== BEGIN tests for class 'Debugger' ===*/
		void testCallbackBundleReceived();
		/*=== END   tests for class 'Debugger' ===*/

		void setUp();
		void tearDown();


		CPPUNIT_TEST_SUITE(DebuggerTest);
			CPPUNIT_TEST(testCallbackBundleReceived);
		CPPUNIT_TEST_SUITE_END();
};
#endif /* DEBUGGERTEST_HH */
