/*
 * DiscoveryBlockTestSuite.h
 *
 *  Created on: 27.02.2009
 *      Author: morgenro
 */

#ifndef DISCOVERYBLOCKTESTSUITE_H_
#define DISCOVERYBLOCKTESTSUITE_H_

#include "config.h"

#ifdef USE_EMMA_CODE

#include "emma/DiscoverBlockFactory.h"

namespace dtn
{
namespace testsuite
{
	class DiscoveryBlockTestSuite
	{
		public:
			DiscoveryBlockTestSuite();

			~DiscoveryBlockTestSuite();

			bool runAllTests();

		private:
			bool createTest();
			bool parseTest();
			bool coordTest();

			emma::DiscoverBlockFactory *m_dfac;
	};
}
}

#endif

#endif /* DISCOVERYBLOCKTESTSUITE_H_ */
