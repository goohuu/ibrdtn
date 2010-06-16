/*
 * StringBundle.h
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#ifndef STRINGBUNDLE_H_
#define STRINGBUNDLE_H_

#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include <iostream>

namespace dtn
{
	namespace api
	{
		/**
		 * This class could be used to send string data through the
		 * bundle protocol.
		 */
		class StringBundle : public Bundle
		{
		public:
			/**
			 * Constructor need a destination.
			 */
			StringBundle(dtn::data::EID destination);

			/**
			 * Destruktor.
			 */
			virtual ~StringBundle();

			/**
			 * Append a string to the data block.
			 */
			void append(string data);

		private:
			dtn::data::PayloadBlock &_payload;
		};
	}
}

#endif /* STRINGBUNDLE_H_ */
