/*
 * SerializeTestSuite.h
 *
 *  Created on: 14.09.2009
 *      Author: morgenro
 */

#ifndef SERIALIZETESTSUITE_H_
#define SERIALIZETESTSUITE_H_

#include "ibrdtn/config.h"


namespace dtn
{
	namespace testsuite
	{
		class SerializeTestSuite
		{
		public:
			SerializeTestSuite();
			virtual ~SerializeTestSuite();

			bool runAllTests();

		private:
			bool BundleString();
			bool DiscoveryAnnouncement();
			bool DiscoveryService();
			bool SDNV();
		};
	}
}

#endif /* SERIALIZETESTSUITE_H_ */
