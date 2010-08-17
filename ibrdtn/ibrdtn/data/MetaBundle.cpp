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
		MetaBundle::MetaBundle()
		 : BundleID(), lifetime(), destination(), reportto(),
		   custodian(), appdatalength(0), procflags(0)
		{
		}

		MetaBundle::MetaBundle(const dtn::data::Bundle &b)
		 : BundleID(b), lifetime(b._lifetime), destination(b._destination), reportto(b._reportto),
		   custodian(b._custodian), appdatalength(b._appdatalength), procflags(b._procflags)
		{
		}

		MetaBundle::~MetaBundle()
		{}

		int MetaBundle::getPriority() const
		{
			// read priority
			if (procflags & dtn::data::Bundle::PRIORITY_BIT1)
			{
				return 0;
			}
			else
			{
				if (procflags & dtn::data::Bundle::PRIORITY_BIT2) return 1;
				else return -1;
			}
		}
	}
}

#endif /* METABUNDLE_CPP_ */
