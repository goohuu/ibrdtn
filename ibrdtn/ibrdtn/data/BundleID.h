/*
 * BundleID.h
 *
 *  Created on: 01.09.2009
 *      Author: morgenro
 */

#ifndef BUNDLEID_H_
#define BUNDLEID_H_

#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/Bundle.h"

namespace dtn
{
	namespace data
	{
		class BundleID
		{
		public:
			BundleID();
			BundleID(const dtn::data::Bundle &b);
			BundleID(EID source, size_t timestamp, size_t sequencenumber, bool fragment = false, size_t offset = 0);
			virtual ~BundleID();

			bool operator!=(const BundleID& other) const;
			bool operator==(const BundleID& other) const;
			bool operator<(const BundleID& other) const;
			bool operator>(const BundleID& other) const;

			string toString() const;
			size_t getTimestamp() const;

			friend std::ostream &operator<<(std::ostream &stream, const BundleID &obj);
			friend std::istream &operator>>(std::istream &stream, BundleID &obj);

		private:
			dtn::data::EID _source;
			size_t _timestamp;
			size_t _sequencenumber;

			bool _fragment;
			size_t _offset;
		};
	}
}

#endif /* BUNDLEID_H_ */
