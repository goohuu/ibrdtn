/*
 * BLOBBundle.h
 *
 *  Created on: 25.01.2010
 *      Author: morgenro
 */

#ifndef BLOBBUNDLE_H_
#define BLOBBUNDLE_H_

#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrcommon/data/BLOB.h"
#include <iostream>

namespace dtn
{
	namespace api
	{
		/**
		 * This class could be used to send string data through the
		 * bundle protocol.
		 */
		class BLOBBundle : public Bundle
		{
		public:
			/**
			 * Constructor need a destination.
			 */
			BLOBBundle(dtn::data::EID destination, ibrcommon::BLOB::Reference &ref);

			/**
			 * Destruktor.
			 */
			virtual ~BLOBBundle();

		private:
			dtn::data::PayloadBlock *_payload;
		};
	}
}

#endif /* BLOBBUNDLE_H_ */
