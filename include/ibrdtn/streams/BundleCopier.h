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
#include "ibrcommon/data/BLOB.h"

namespace dtn
{
	namespace streams
	{
		class BundleCopier : public BundleHandler
		{
			enum State
			{
				IDLE = 0,
				PRIMARY_BLOCK = 1,
				PAYLOAD_BLOCK_HEADER = 2,
				PAYLOAD_BLOCK_DATA = 3,
				EXTENSION_BLOCK = 4,
				ADM_BLOCK = 5,
				FINISHED = 6
			};

		public:
			BundleCopier(dtn::data::Bundle &b);
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
			ATTRIBUTES _next;
			dtn::data::Bundle &_bundle;
			dtn::data::Block *_block;
			pair<size_t, size_t> _eids[4];
			dtn::data::Dictionary _dict;

			int _blockref;

			State _state;
			size_t _last_blocksize;
		};
	}
}

#endif /* BUNDLECOPIER_H_ */
