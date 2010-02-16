/*
 * MetaBundle.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef METABUNDLE_CPP_
#define METABUNDLE_CPP_

#include "routing/MetaBundle.h"

namespace dtn
{
	namespace routing
	{
		MetaBundle::MetaBundle(const dtn::data::Bundle &b)
		 : BundleID(b), lifetime(b._lifetime), destination(b._destination)
		{
		}

		MetaBundle::~MetaBundle()
		{}
	}
}

#endif /* METABUNDLE_CPP_ */
