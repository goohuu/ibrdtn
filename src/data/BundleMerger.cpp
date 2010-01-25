/*
 * BundleMerger.cpp
 *
 *  Created on: 25.01.2010
 *      Author: morgenro
 */

#include "ibrdtn/data/BundleMerger.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Exceptions.h"

namespace dtn
{
	namespace data
	{
		BundleMerger::Container::Container(dtn::data::Bundle &b, ibrcommon::BLOB::Reference &ref)
		 : _bundle(b), _blob(ref)
		{
			// check if the given bundle is a fragment
			if (!(_bundle._procflags & dtn::data::Bundle::FRAGMENT)) throw dtn::exceptions::FragmentationException("This bundle is not a fragment!");

			// remove all block of the copy
			_bundle.clearBlocks();

			// mark the copy as non-fragment
			_bundle._procflags -= dtn::data::Bundle::FRAGMENT;

			// add a new payloadblock
			dtn::data::PayloadBlock *payload = new dtn::data::PayloadBlock(_blob);
			_bundle.addBlock(payload);
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

		BundleMerger::Container &operator<<(BundleMerger::Container &c, const dtn::data::Bundle &obj)
		{
			if (	(c._bundle._timestamp != obj._timestamp) ||
					(c._bundle._sequencenumber != obj._sequencenumber) ||
					(c._bundle._source != obj._source) )
				throw dtn::exceptions::FragmentationException("This fragment does not belongs to the others.");

			ibrcommon::MutexLock l(c._blob);
			(*c._blob).seekp(obj._fragmentoffset);

			const list<dtn::data::Block*> blocks = obj.getBlocks(dtn::data::PayloadBlock::BLOCK_TYPE);

			for (list<dtn::data::Block*>::const_iterator iter = blocks.begin(); iter != blocks.end(); iter++)
			{
				ibrcommon::BLOB::Reference ref = (*iter)->getBLOB();
				ibrcommon::MutexLock reflock(ref);

				size_t ret = 0;
				istream &s = (*ref);
				s.seekg(0);

				while (!s.eof())
				{
					char buf;
					s.get(buf);
					(*c._blob).put(buf);
					ret++;
				}
			}

			return c;
		}

		BundleMerger::Container BundleMerger::getContainer(dtn::data::Bundle &b)
		{
			ibrcommon::BLOB::Reference ref = ibrcommon::TmpFileBLOB::create();
			return Container(b, ref);
		}

		BundleMerger::Container BundleMerger::getContainer(dtn::data::Bundle &b, ibrcommon::BLOB::Reference &ref)
		{
			return Container(b, ref);
		}
	}
}
