/*
 * BundleFactory.h
 *
 *  Created on: 03.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef BUNDLECOPIER_H_
#define BUNDLECOPIER_H_

#include "ibrdtn/streams/BundleHandler.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BLOBManager.h"

namespace dtn
{
	namespace streams
	{
		class BundleCopier : public BundleHandler
		{
		public:
			BundleCopier(dtn::blob::BLOBManager &blobmanager, dtn::data::Bundle &b);
			virtual ~BundleCopier();

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

		private:
			blob::BLOBManager &_blobmanager;
			ATTRIBUTES _next;
			dtn::data::Bundle &_bundle;
			dtn::data::Block *_block;
			pair<size_t, size_t> _eids[4];
			dtn::data::Dictionary _dict;

			int _blockref;
		};
	}
}

#endif /* BUNDLECOPIER_H_ */
