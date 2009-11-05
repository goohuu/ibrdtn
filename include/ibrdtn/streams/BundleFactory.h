/*
 * BundleFactory.h
 *
 *  Created on: 03.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef BUNDLEFACTORY_H_
#define BUNDLEFACTORY_H_

#include "ibrdtn/streams/BundleHandler.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BLOBManager.h"

namespace dtn
{
	namespace streams
	{
		class BundleFactory : public BundleHandler
		{
		public:
			BundleFactory(blob::BLOBManager &blobmanager);
			virtual ~BundleFactory();

			void beginBundle(char version);
			void endBundle();
			void beginAttribute(ATTRIBUTES attr);
			void endAttribute(size_t value);
			void endAttribute(char value);
			void endAttribute(char *data, size_t size);
			void beginBlock(char type);
			void endBlock();
			void beginBlob();
			void dataBlob(char *data, size_t length);
			void endBlob(size_t size);

			dtn::data::Bundle getBundle();

		private:
			blob::BLOBManager &_blobmanager;
			ATTRIBUTES _next;
			dtn::data::Bundle _bundle;
			dtn::data::Block *_block;
			pair<size_t, size_t> _eids[4];
			dtn::data::Dictionary _dict;

			int _blockref;
		};
	}
}

#endif /* BUNDLEFACTORY_H_ */
