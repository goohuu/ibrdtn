/*
 * BundleString.h
 *
 *  Created on: 11.09.2009
 *      Author: morgenro
 */

#ifndef BUNDLESTRING_H_
#define BUNDLESTRING_H_

#include "ibrdtn/config.h"
#include <string>

namespace dtn
{
	namespace data
	{
		class BundleString : public std::string
		{
		public:
			BundleString(std::string value);
			BundleString();
			virtual ~BundleString();

			size_t getLength() const;

		private:
			friend std::ostream &operator<<(std::ostream &stream, const BundleString &bstring);
			friend std::istream &operator>>(std::istream &stream, BundleString &bstring);
		};
	}
}

#endif /* BUNDLESTRING_H_ */
