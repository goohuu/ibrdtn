/*
 * BLOBReference.h
 *
 *  Created on: 28.05.2009
 *      Author: morgenro
 */

#ifndef BLOBREFERENCE_H_
#define BLOBREFERENCE_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/BLOBManager.h"
#include "ibrdtn/utils/Mutex.h"

namespace dtn
{
	namespace blob
	{
		class BLOBReference : public dtn::utils::Mutex
		{
			friend class BLOBManager;
		public:
			BLOBReference(const BLOBReference &ref);
			virtual ~BLOBReference();

			size_t read(char *data, size_t offset, size_t size);
			size_t read(ostream &stream);

			size_t write(const char *data, size_t offset, size_t size);
			size_t write(size_t offset, istream &stream);

			size_t append(const char* data, size_t size);
			size_t append(BLOBReference &ref);

			void clear();

			BLOBReference copy();
			pair<BLOBReference, BLOBReference> split(size_t split_position) const;

			size_t getSize() const;

		protected:
			BLOBReference();
			BLOBReference(BLOBManager *manager, string key);
			string _key;

		private:
			BLOBManager *_manager;


		};
	}
}

#endif /* BLOBREFERENCE_H_ */
