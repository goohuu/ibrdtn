/*
 * UDPConnection.cpp
 *
 *  Created on: 25.09.2009
 *      Author: morgenro
 */

#include "net/UDPConnection.h"
#include "core/Node.h"

namespace dtn
{
	namespace net
	{
		UDPConnection::UDPConnection(UDPConvergenceLayer &cl, const dtn::core::Node &node)
		 : _cl(cl), _node(node)
		{
		}

		UDPConnection::~UDPConnection()
		{
		}

		void UDPConnection::write(const dtn::data::Bundle &bundle)
		{
			_cl.transmit(bundle, _node);
		}

		void UDPConnection::read(dtn::data::Bundle &bundle)
		{
			_cl.receive(bundle);
		}
	}
}
