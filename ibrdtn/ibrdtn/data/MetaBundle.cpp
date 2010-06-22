/*
 * MetaBundle.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef METABUNDLE_CPP_
#define METABUNDLE_CPP_

#include "ibrdtn/data/MetaBundle.h"

namespace dtn
{
	namespace data
	{
		MetaBundle::MetaBundle(const dtn::data::Bundle &b)
		 : BundleID(b), lifetime(b._lifetime), destination(b._destination), reportto(b._reportto),
		   custodian(b._custodian), appdatalength(b._appdatalength), procflags(b._procflags)
		{
		}

		MetaBundle::~MetaBundle()
		{}
	}
}

#endif /* METABUNDLE_CPP_ */
