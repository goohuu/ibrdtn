/*
 * FakeBundleHandler.h
 *
 *  Created on: 03.07.2009
 *      Author: morgenro
 */

#ifndef FAKEBUNDLEHANDLER_H_
#define FAKEBUNDLEHANDLER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/streams/BundleHandler.h"

namespace dtn
{
	namespace streams
	{
		class FakeBundleHandler : public BundleHandler
		{
		public:
			FakeBundleHandler() {};
			virtual ~FakeBundleHandler() {};

			void beginBundle(char version) {};
			void endBundle() {};
			void beginAttribute(ATTRIBUTES attr) {};
			void endAttribute(char value) {};
			void endAttribute(char *data, size_t size) {};
			void endAttribute(size_t value) {};
			void beginBlock(char type) {};
			void endBlock() {};
			void beginBlob() {};
			void dataBlob(char *data, size_t length) {};
			void endBlob(size_t size) {};
		};
	}
}

#endif /* FAKEBUNDLEHANDLER_H_ */
