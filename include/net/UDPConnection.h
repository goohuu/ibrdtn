/*
 * UDPConnection.h
 *
 *  Created on: 25.09.2009
 *      Author: morgenro
 */

#ifndef UDPCONNECTION_H_
#define UDPCONNECTION_H_

#include "ibrdtn/default.h"
#include "net/UDPConvergenceLayer.h"
#include "net/BundleConnection.h"
#include "core/Node.h"

namespace dtn
{
	namespace net
	{
		class UDPConnection : public BundleConnection
		{
		public:
			UDPConnection(UDPConvergenceLayer &cl, const dtn::core::Node &node);
			virtual ~UDPConnection();

			void write(const dtn::data::Bundle &bundle);
			void read(dtn::data::Bundle &bundle);

		private:
			UDPConvergenceLayer &_cl;
			dtn::core::Node _node;
		};
	}
}

#endif /* UDPCONNECTION_H_ */
