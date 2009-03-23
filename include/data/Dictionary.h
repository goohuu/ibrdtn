#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include <string>
#include "Exceptions.h"
#include <map>

using namespace std;

namespace dtn
{
	namespace data
	{
		/**
		 * This class manages a dictionary of a bundles primary block.
		 */
		class Dictionary
		{
		public:
			/**
			 * constructor
			 */
			Dictionary();

			/**
			 * destructor
			 */
			virtual ~Dictionary();

			/**
			 * Add a new entry to the dictionary.
			 */
			pair<unsigned int, unsigned int> add(string uri);

			/**
			 * Return a full uri from the dictionary.
			 */
			string get(unsigned int scheme, unsigned int ssp) const;

			/**
			 * Return a entry of the dictionary.
			 */
			string get(unsigned int pos) const;

			/**
			 * Return the position of a entry in the data array.
			 */
			unsigned int get(string entry) const;

			/**
			 * Read a dictionary from a data array.
			 */
			void read(unsigned char *data, unsigned int length);

			/**
			 * Return the length of the dictionary data array.
			 */
			unsigned int getLength() const;

			/**
			 * Return the count of the entrys in the dictionary.
			 */
			unsigned int getSize() const;

			/**
			 * Write the dictionary to a given data array and returns the count
			 * of written bytes.
			 * @return Count of written bytes.
			 */
			unsigned int write(unsigned char *data) const;

			/**
			 * Split a URI into Scheme and SSP
			 * @param uri The URI to split.
			 * @return A pair of Scheme and SSP.
			 */
			static pair<string, string> split(string uri) throw (exceptions::InvalidDataException);

		private:
			/**
			 * Check if a entry already exists in the dictionary.
			 */
			bool exists(string entry);

			/**
			 * Sort function for the map.
			 */
			struct m_positionSort {
				bool operator()( unsigned int pos1, unsigned int pos2 ) const {
					return (pos1 < pos2);
				}
			};

			map<unsigned int, string, m_positionSort> m_entrys;
		};
	}
}

#endif /*DICTIONARY_H_*/
