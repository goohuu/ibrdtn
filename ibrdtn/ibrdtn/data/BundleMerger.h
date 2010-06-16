/*
 * BundleMerger.h
 *
 *  Created on: 25.01.2010
 *      Author: morgenro
 */

#ifndef BUNDLEMERGER_H_
#define BUNDLEMERGER_H_

#include "ibrcommon/data/BLOB.h"
#include "ibrdtn/data/Bundle.h"

namespace dtn
{
	namespace data
	{
		class BundleMerger
		{
		public:
			class Container
			{
			public:
				Container(dtn::data::Bundle &b, ibrcommon::BLOB::Reference &ref);
				~Container();
				bool isComplete();

				Bundle getBundle();

				friend Container &operator<<(Container &c, dtn::data::Bundle &obj);

			private:
				dtn::data::Bundle _bundle;
				ibrcommon::BLOB::Reference _blob;
			};

			static Container getContainer(dtn::data::Bundle &b);
			static Container getContainer(dtn::data::Bundle &b, ibrcommon::BLOB::Reference &ref);
		};
	}
}


#endif /* BUNDLEMERGER_H_ */
