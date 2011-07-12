/*
 * BundleMerger.cpp
 *
 *  Created on: 25.01.2010
 *      Author: morgenro
 */

#include "ibrdtn/config.h"
#include "ibrdtn/data/BundleMerger.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Exceptions.h"
#include <ibrcommon/thread/MutexLock.h>

namespace dtn
{
	namespace data
	{
		BundleMerger::Container::Container(dtn::data::Bundle &b, ibrcommon::BLOB::Reference &ref)
		 : _bundle(b), _blob(ref)
		{
			// check if the given bundle is a fragment
			if (!(_bundle.get(dtn::data::Bundle::FRAGMENT)))
			{
				throw ibrcommon::Exception("This bundle is not a fragment!");
			}

			// remove all block of the copy
			_bundle.clearBlocks();

			// mark the copy as non-fragment
			_bundle.set(dtn::data::Bundle::FRAGMENT, false);

			// add a new payloadblock
			_bundle.push_back(_blob);
		}

		BundleMerger::Container::~Container()
		{

		}

		bool BundleMerger::Container::isComplete()
		{
			return false;
		}

		Bundle BundleMerger::Container::getBundle()
		{
			return _bundle;
		}

		BundleMerger::Container &operator<<(BundleMerger::Container &c, dtn::data::Bundle &obj)
		{
			if (	(c._bundle._timestamp != obj._timestamp) ||
					(c._bundle._sequencenumber != obj._sequencenumber) ||
					(c._bundle._source != obj._source) )
				throw ibrcommon::Exception("This fragment does not belongs to the others.");

			ibrcommon::BLOB::iostream stream = c._blob.iostream();
			(*stream).seekp(obj._fragmentoffset);

			dtn::data::PayloadBlock &p = obj.getBlock<dtn::data::PayloadBlock>();

			ibrcommon::BLOB::Reference ref = p.getBLOB();
			ibrcommon::BLOB::iostream s = ref.iostream();

			size_t ret = 0;
			s->seekg(0);

			while (!s->eof())
			{
				char buf;
				s->get(buf);
				stream->put(buf);
				ret++;
			}

			return c;
		}

		BundleMerger::Container BundleMerger::getContainer(dtn::data::Bundle &b)
		{
			ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();
			return Container(b, ref);
		}

		BundleMerger::Container BundleMerger::getContainer(dtn::data::Bundle &b, ibrcommon::BLOB::Reference &ref)
		{
			return Container(b, ref);
		}
	}
}
