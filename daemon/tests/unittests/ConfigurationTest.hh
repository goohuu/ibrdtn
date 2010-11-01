/* $Id: templateengine.py 2241 2006-05-22 07:58:58Z fischer $ */

///
/// @file        ConfigurationTest.hh
/// @brief       CPPUnit-Tests for class Configuration
/// @author      Author Name (email@mail.address)
/// @date        Created at 2010-11-01
/// 
/// @version     $Revision: 2241 $
/// @note        Last modification: $Date: 2006-05-22 09:58:58 +0200 (Mon, 22 May 2006) $
///              by $Author: fischer $
///

 
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifndef CONFIGURATIONTEST_HH
#define CONFIGURATIONTEST_HH
class ConfigurationTest : public CppUnit::TestFixture {
	private:
	public:
		/*=== BEGIN tests for class 'Configuration' ===*/
		void testGetInstance();
		void testLoad();
		void testParams();
		void testGetNodename();
		void testGetTimezone();
		void testGetPath();
		void testGetUID();
		void testGetGID();
		void testDoAPI();
		void testGetAPIInterface();
		void testGetAPISocket();
		void testVersion();
		void testGetNotifyCommand();
		void testGetStorage();
		void testGetLimit();
		/*=== BEGIN tests for class 'Discovery' ===*/
		void testDiscoveryLoad();
		void testDiscoveryEnabled();
		void testDiscoveryAnnounce();
		void testDiscoveryShortbeacon();
		void testDiscoveryVersion();
		void testDiscoveryAddress();
		void testDiscoveryPort();
		void testDiscoveryTimeout();
		/*=== END   tests for class 'Discovery' ===*/

		/*=== BEGIN tests for class 'Statistic' ===*/
		void testStatisticLoad();
		void testStatisticEnabled();
		void testStatisticLogfile();
		void testStatisticType();
		void testStatisticInterval();
		void testStatisticAddress();
		void testStatisticPort();
		/*=== END   tests for class 'Statistic' ===*/

		/*=== BEGIN tests for class 'Debug' ===*/
		void testDebugLoad();
		void testDebugLevel();
		void testDebugEnabled();
		void testDebugQuiet();
		/*=== END   tests for class 'Debug' ===*/

		/*=== BEGIN tests for class 'Logger' ===*/
		void testLoggerLoad();
		void testQuiet();
		void testOptions();
		void testOutput();
		/*=== END   tests for class 'Logger' ===*/

		/*=== BEGIN tests for class 'Network' ===*/
		void testNetworkLoad();
		void testGetInterfaces();
		void testGetStaticNodes();
		void testGetStaticRoutes();
		void testGetRoutingExtension();
		void testDoForwarding();
		void testGetTCPOptionNoDelay();
		void testGetTCPChunkSize();
		/*=== END   tests for class 'Network' ===*/

		void testGetDiscovery();
		void testGetStatistic();
		void testGetDebug();
		void testGetLogger();
		void testGetNetwork();
		/*=== END   tests for class 'Configuration' ===*/

		void setUp();
		void tearDown();


		CPPUNIT_TEST_SUITE(ConfigurationTest);
			CPPUNIT_TEST(testGetInstance);
			CPPUNIT_TEST(testLoad);
			CPPUNIT_TEST(testParams);
			CPPUNIT_TEST(testGetNodename);
			CPPUNIT_TEST(testGetTimezone);
			CPPUNIT_TEST(testGetPath);
			CPPUNIT_TEST(testGetUID);
			CPPUNIT_TEST(testGetGID);
			CPPUNIT_TEST(testDoAPI);
			CPPUNIT_TEST(testGetAPIInterface);
			CPPUNIT_TEST(testGetAPISocket);
			CPPUNIT_TEST(testVersion);
			CPPUNIT_TEST(testGetNotifyCommand);
			CPPUNIT_TEST(testGetStorage);
			CPPUNIT_TEST(testGetLimit);
			CPPUNIT_TEST(testDiscoveryLoad);
			CPPUNIT_TEST(testDiscoveryEnabled);
			CPPUNIT_TEST(testDiscoveryAnnounce);
			CPPUNIT_TEST(testDiscoveryShortbeacon);
			CPPUNIT_TEST(testDiscoveryVersion);
			CPPUNIT_TEST(testDiscoveryAddress);
			CPPUNIT_TEST(testDiscoveryPort);
			CPPUNIT_TEST(testDiscoveryTimeout);
			CPPUNIT_TEST(testStatisticLoad);
			CPPUNIT_TEST(testStatisticEnabled);
			CPPUNIT_TEST(testStatisticLogfile);
			CPPUNIT_TEST(testStatisticType);
			CPPUNIT_TEST(testStatisticInterval);
			CPPUNIT_TEST(testStatisticAddress);
			CPPUNIT_TEST(testStatisticPort);
			CPPUNIT_TEST(testDebugLoad);
			CPPUNIT_TEST(testDebugLevel);
			CPPUNIT_TEST(testDebugEnabled);
			CPPUNIT_TEST(testDebugQuiet);
			CPPUNIT_TEST(testLoggerLoad);
			CPPUNIT_TEST(testQuiet);
			CPPUNIT_TEST(testOptions);
			CPPUNIT_TEST(testOutput);
			CPPUNIT_TEST(testNetworkLoad);
			CPPUNIT_TEST(testGetInterfaces);
			CPPUNIT_TEST(testGetStaticNodes);
			CPPUNIT_TEST(testGetStaticRoutes);
			CPPUNIT_TEST(testGetRoutingExtension);
			CPPUNIT_TEST(testDoForwarding);
			CPPUNIT_TEST(testGetTCPOptionNoDelay);
			CPPUNIT_TEST(testGetTCPChunkSize);
			CPPUNIT_TEST(testGetDiscovery);
			CPPUNIT_TEST(testGetStatistic);
			CPPUNIT_TEST(testGetDebug);
			CPPUNIT_TEST(testGetLogger);
			CPPUNIT_TEST(testGetNetwork);
		CPPUNIT_TEST_SUITE_END();
};
#endif /* CONFIGURATIONTEST_HH */
