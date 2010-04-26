/*
 * Dictionary.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include "ibrdtn/streams/BundleWriter.h"
#include "ibrdtn/data/EID.h"
#include <list>
#include <sstream>
#include <list>

using namespace std;
using namespace dtn::streams;

namespace dtn
{
	namespace data
	{
		class Dictionary
		{
		public:
			/**
			 * create a empty dictionary
			 */
			Dictionary();

			/**
			 * create a dictionary with a given bytearray
			 */
			Dictionary(const char *data, size_t size);

			/**
			 * destructor
			 */
			virtual ~Dictionary();

			/**
			 * overwrites the dictionary
			 */
			void read(const char *data, size_t size);

			/**
			 * add a eid to the dictionary
			 */
			void add(const EID &eid);

			/**
			 * add a list of eids to the dictionary
			 */
			void add(const list<EID> &eids);

			/**
			 * return the eid for the reference [scheme,ssp]
			 */
			EID get(size_t scheme, size_t ssp);

			/**
			 * clear the dictionary
			 */
			void clear();

			/**
			 * returns the size of the bytearray
			 */
			size_t getSize() const;

			/**
			 * returns the references of the given eid
			 */
			pair<size_t, size_t> getRef(const EID &eid) const;

			/**
			 * write a reference (scheme + ssp)
			 */
			size_t writeRef( BundleWriter &writer, const EID &eid ) const;

			/**
			 * write the length of the bytearray
			 */
			size_t writeLength( BundleWriter &writer ) const;

			/**
			 * write the bytearray
			 */
			size_t writeByteArray( BundleWriter &writer ) const;

		private:
			/**
			 * copy constructor
			 */
			Dictionary(const Dictionary &d) {};

			/**
			 * assign operator
			 */
			Dictionary operator=(const Dictionary &d) { return Dictionary(); };

			stringstream _bytestream;
		};
	}
}

#endif /* DICTIONARY_H_ */