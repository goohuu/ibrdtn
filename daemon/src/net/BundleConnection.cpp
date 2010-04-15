/*
 * BundleConnection.cpp
 *
 *  Created on: 26.10.2009
 *      Author: morgenro
 */

#include "net/BundleConnection.h"

namespace dtn
{
	namespace net
	{
		BundleConnection::~BundleConnection()
		{}

		BundleConnection &operator<<(BundleConnection &c, const dtn::data::Bundle &obj)
		{
			c.write(obj);
			return c;
		}

		bool BundleConnection::isBusy() const
		{ return false; }
	}
}
