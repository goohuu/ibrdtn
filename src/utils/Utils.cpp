#include "ibrdtn/utils/Utils.h"
#include "ibrcommon/data/BLOB.h"
#include "ibrdtn/data/Exceptions.h"

namespace dtn
{
	namespace utils
	{
		int Utils::timezone = 0;

		size_t Utils::get_current_dtn_time()
		{
			time_t rawtime = time(NULL);
			tm * ptm;

			ptm = gmtime ( &rawtime );

			// timezone
			int offset = Utils::timezone * 3600;

			return (mktime(ptm) - 946681200) + offset;
		}

		vector<string> Utils::tokenize(string token, string data)
		{
			vector<string> l;
			string value;

			// Skip delimiters at beginning.
			string::size_type lastPos = data.find_first_not_of(token, 0);
			// Find first "non-delimiter".
			string::size_type pos     = data.find_first_of(token, lastPos);

			while (string::npos != pos || string::npos != lastPos)
			{
				// Found a token, add it to the vector.
				value = data.substr(lastPos, pos - lastPos);
				l.push_back(value);
				// Skip delimiters.  Note the "not_of"
				lastPos = data.find_first_not_of(token, pos);
				// Find next "non-delimiter"
				pos = data.find_first_of(token, lastPos);
			}

			return l;
		}

		/**
		 * calculate the distance between two coordinates. (Latitude, Longitude)
		 */
		 double Utils::distance(double lat1, double lon1, double lat2, double lon2)
		 {
			const double r = 6371; //km

			double dLat = toRad( (lat2-lat1) );
			double dLon = toRad( (lon2-lon1) );

			double a = 	sin(dLat/2) * sin(dLat/2) +
						cos(toRad(lat1)) * cos(toRad(lat2)) *
						sin(dLon/2) * sin(dLon/2);
			double c = 2 * atan2(sqrt(a), sqrt(1-a));
			return r * c;
		 }

		 const double Utils::pi = 3.14159;

		 double Utils::toRad(double value)
		 {
			return value * pi / 180;
		 }

		dtn::data::CustodySignalBlock* Utils::getCustodySignalBlock(const dtn::data::Bundle &bundle)
		{
			const std::list<dtn::data::Block*> blocks = bundle.getBlocks();
			std::list<dtn::data::Block*>::const_iterator iter = blocks.begin();

			while (iter != blocks.end())
			{
				dtn::data::CustodySignalBlock *block = dynamic_cast<dtn::data::CustodySignalBlock*>(*iter);
				if (block != NULL) return block;
			}

			return NULL;
		}

		dtn::data::StatusReportBlock* Utils::getStatusReportBlock(const dtn::data::Bundle &bundle)
		{
			const list<dtn::data::Block*> blocks = bundle.getBlocks();
			list<dtn::data::Block*>::const_iterator iter = blocks.begin();

			while (iter != blocks.end())
			{
				dtn::data::StatusReportBlock *block = dynamic_cast<dtn::data::StatusReportBlock*>(*iter);
				if (block != NULL) return block;
			}

			return NULL;
		}

		dtn::data::PayloadBlock* Utils::getPayloadBlock(const dtn::data::Bundle &bundle)
		{
			const list<dtn::data::Block*> blocks = bundle.getBlocks();
			list<dtn::data::Block*>::const_iterator iter = blocks.begin();

			while (iter != blocks.end())
			{
				dtn::data::PayloadBlock *payload = dynamic_cast<dtn::data::PayloadBlock*>(*iter);
				if (payload != NULL) return payload;
			}

			return NULL;
		}

//		pair<dtn::data::PayloadBlock*, dtn::data::PayloadBlock*> Utils::split(dtn::data::PayloadBlock *block, size_t split_position)
//		{
//			const ibrcommon::BLOB::Reference b = block->getBLOB::Reference();
//			pair<ibrcommon::BLOB::Reference, ibrcommon::BLOB::Reference> refpair = b.split(split_position);
//
//			dtn::data::PayloadBlock *p1 = new dtn::data::PayloadBlock(refpair.first);
//			dtn::data::PayloadBlock *p2 = new dtn::data::PayloadBlock(refpair.second);
//
//			return make_pair(p1, p2);
//		}
//
//		bool Utils::compareFragments(const dtn::data::Bundle &first, const dtn::data::Bundle &second)
//		{
//			// if the offset of the first bundle is lower than the second...
//			if (first._fragmentoffset < second._fragmentoffset)
//			{
//				return true;
//			}
//			else
//			{
//				return false;
//			}
//		}

//		dtn::data::Bundle Utils::merge(dtn::data::Bundle &destination, const dtn::data::Bundle &source)
//		{
//			if (!(destination._procflags & dtn::data::Bundle::FRAGMENT))
//			{
//				throw dtn::exceptions::FragmentationException("At least one of the bundles isn't a fragment.");
//			}
//
//			if (!(source._procflags & dtn::data::Bundle::FRAGMENT))
//			{
//				throw dtn::exceptions::FragmentationException("At least one of the bundles isn't a fragment.");
//			}
//
//			// check that they belongs together
//			if (( destination._timestamp != source._timestamp ) ||
//				( destination._sequencenumber != source._sequencenumber ) ||
//				( destination._lifetime != source._lifetime ) ||
//				( destination._appdatalength != source._appdatalength ) ||
//				( destination._source != source._source ) )
//			{
//				// exception
//				throw dtn::exceptions::FragmentationException("This fragments don't belongs together.");
//			}
//
//			// checks complete, now merge the blocks
//			dtn::data::PayloadBlock *payload1 = Utils::getPayloadBlock( destination );
//			dtn::data::PayloadBlock *payload2 = Utils::getPayloadBlock( source );
//
//			// TODO: copy blocks other than the payload block!
//
//			unsigned int endof1 = destination._fragmentoffset + payload1->getBLOBReference().getSize();
//
//			if (endof1 < source._fragmentoffset)
//			{
//				// this aren't adjacency fragments
//				throw dtn::exceptions::FragmentationException("This aren't adjacency fragments and can't be merged.");
//			}
//
//			// relative offset of payload1 to payload2
//			unsigned int p2offset = source._fragmentoffset - destination._fragmentoffset;
//
//			// append the payload of fragment2 at the end of fragment1
//			ibrcommon::BLOBReference ref = payload2->getBLOBReference();
//			payload1->getBLOBReference().append( ref );
//
//			size_t payload_s = payload1->getBLOBReference().getSize();
//
//			// if the bundle is complete return a non-fragmented bundle instead of the fragment
//			if (payload_s == destination._appdatalength)
//			{
//				// remove the fragment fields of the bundle data
//				if (destination._procflags & dtn::data::Bundle::FRAGMENT) destination._procflags -= dtn::data::Bundle::FRAGMENT;
//			}
//
//			return destination;
//		}

//		dtn::data::Bundle Utils::merge(std::list<dtn::data::Bundle> &bundles)
//		{
//			// no bundle, raise a exception
//			if (bundles.size() <= 1)
//			{
//				throw dtn::exceptions::FragmentationException("None or only one item in the list.");
//			}
//
//			// sort the fragments
//			bundles.sort(Utils::compareFragments);
//
//			// take a copy of the first bundle as base and merge it with the others
//			dtn::data::Bundle first = bundles.front();
//			bundles.pop_front();
//			dtn::data::Bundle second = bundles.front();
//			dtn::data::Bundle bundle;
//
//			try {
//				// the first merge creates a new bundle object
//				bundle = merge(first, second);
//				bundles.pop_front();
//			} catch (exceptions::FragmentationException ex) {
//				bundles.push_front(first);
//				throw ex;
//			}
//
//			if (bundle._procflags & dtn::data::Bundle::FRAGMENT)
//			{
//				// put the new fragment into the list
//				bundles.push_back(bundle);
//
//				// call merge recursive
//				return merge(bundles);
//			}
//			else
//			{
//				return bundle;
//			}
//		}
	}
}
