/*
 * BLOBReference.cpp
 *
 *  Created on: 28.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/BLOBReference.h"

namespace dtn
{
	namespace blob
	{
		BLOBReference::BLOBReference()
		 : _key("null"), _manager(NULL)
		{

		}

		BLOBReference::BLOBReference(const BLOBReference &ref)
		 : _key(ref._key), _manager(ref._manager)
		{
			_manager->linkReference(*this);
		}

		BLOBReference::BLOBReference(BLOBManager *manager, string key)
		 : _key(key), _manager(manager)
		{
			_manager->linkReference(*this);
		}

		BLOBReference::~BLOBReference()
		{
			if (_key != "null")
			{
				_manager->unlinkReference(*this);
			}
		}

		size_t BLOBReference::read(char *data, size_t offset, size_t size)
		{
			return _manager->read(*this, data, offset, size);
		}

		size_t BLOBReference::read(ostream &stream)
		{
			return _manager->read(*this, stream);
		}

		size_t BLOBReference::write(const char *data, size_t offset, size_t size)
		{
			return _manager->write(*this, data, offset, size);
		}

		size_t BLOBReference::write(size_t offset, istream &stream)
		{
			return _manager->write(*this, offset, stream);
		}

		size_t BLOBReference::append(const char* data, size_t size)
		{
			return _manager->append(*this, data, size);
		}

		size_t BLOBReference::append(BLOBReference &ref)
		{
			return _manager->append(*this, ref);
		}

		void BLOBReference::clear()
		{
			_manager->clear(*this);
		}

		size_t BLOBReference::getSize() const
		{
			return _manager->getSize(*this);
		}

		BLOBReference BLOBReference::copy()
		{
			BLOBReference ret = _manager->create();
			_manager->copyData(*this, 0, ret, 0, getSize());
			return ret;
		}

		pair<BLOBReference, BLOBReference> BLOBReference::split(size_t split_position) const
		{
			return _manager->split(*this, split_position);
		}
	}
}
