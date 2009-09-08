/*
 * DefragmentContainer.cpp
 *
 *  Created on: 08.09.2009
 *      Author: morgenro
 */

#include "data/DefragmentContainer.h"
#include "data/BundleFactory.h"

namespace dtn
{
	namespace data
	{
		DefragmentContainer::DefragmentContainer(const Bundle &bundle)
		{
			_application_size = bundle.getInteger(APPLICATION_DATA_LENGTH);
			_timestamp = bundle.getInteger(CREATION_TIMESTAMP);
			_sequencenumber = bundle.getInteger(CREATION_TIMESTAMP_SEQUENCE);
			_source = bundle.getSource();

			_fragments.push_back(bundle);
		}

		DefragmentContainer::~DefragmentContainer()
		{
		}

		bool DefragmentContainer::match(const Bundle &bundle)
		{
			if (_application_size != bundle.getInteger(APPLICATION_DATA_LENGTH)) return false;
			if (_timestamp != bundle.getInteger(CREATION_TIMESTAMP)) return false;
			if (_sequencenumber != bundle.getInteger(CREATION_TIMESTAMP_SEQUENCE)) return false;
			if (_source != bundle.getSource()) return false;

			return true;
		}

		bool DefragmentContainer::isComplete()
		{
			// no bundle, raise a exception
			if (_fragments.size() <= 1)
			{
				return false;
			}

			// sort the fragments
			_fragments.sort(DefragmentContainer::compareFragments);

			// get an iterator for the fragments
			list<Bundle>::iterator iter = _fragments.begin();

			// check the first fragment
			if ((*iter).getInteger(FRAGMENTATION_OFFSET) != 0) return false;
			iter++;

			size_t offset = (*iter).getPayloadBlock()->getLength() + 1;

			while (iter != _fragments.end())
			{
				Bundle &bundle = (*iter);

				if (offset < bundle.getInteger(FRAGMENTATION_OFFSET)) return false;

				offset = bundle.getInteger(FRAGMENTATION_OFFSET) + bundle.getPayloadBlock()->getLength();

				iter++;
			}

			if (offset >= _application_size) return true;

			return false;
		}

		void DefragmentContainer::add(const Bundle &bundle)
		{
			// check for duplicates.
			list<Bundle>::iterator iter = _fragments.begin();

			while (iter != _fragments.end())
			{
				if ( (*iter).getInteger(FRAGMENTATION_OFFSET) == bundle.getInteger(FRAGMENTATION_OFFSET) )
				{
					// bundle found, discard the current fragment.
					return;
				}
				iter++;
			}

			// add the bundle to the fragment list
			_fragments.push_back(bundle);
		}

		Bundle* DefragmentContainer::getBundle()
		{
			// try to merge the fragments
			Bundle *ret = BundleFactory::merge(_fragments);
			_fragments.clear();

			return ret;
		}

		bool DefragmentContainer::compareFragments(const Bundle &first, const Bundle &second)
		{
			// if the offset of the first bundle is lower than the second...
			if (first.getInteger(FRAGMENTATION_OFFSET) < second.getInteger(FRAGMENTATION_OFFSET))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}
