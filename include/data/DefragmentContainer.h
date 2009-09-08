/*
 * DefragmentContainer.h
 *
 *  Created on: 08.09.2009
 *      Author: morgenro
 */

#ifndef DEFRAGMENTCONTAINER_H_
#define DEFRAGMENTCONTAINER_H_

#include "config.h"
#include "data/Bundle.h"
#include <string>
#include <list>

namespace dtn
{
	namespace data
	{
		class DefragmentContainer
		{
		public:
			DefragmentContainer(const Bundle &bundle);
			~DefragmentContainer();

			bool match(const Bundle &bundle);

			bool isComplete();
			void add(const Bundle &bundle);

			Bundle* getBundle();

		private:
			list<Bundle> _fragments;

			size_t _application_size;
			size_t _timestamp;
			size_t _sequencenumber;
			string _source;

			/**
			 * compare method for sorting fragments
			 */
			static bool compareFragments(const Bundle &first, const Bundle &second);
		};
	}
}

#endif /* DEFRAGMENTCONTAINER_H_ */
