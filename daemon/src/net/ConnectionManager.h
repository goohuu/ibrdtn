/*
 * ConnectionManager.h
 *
 *  Created on: 18.09.2009
 *      Author: morgenro
 */

#ifndef CONNECTIONMANAGER_H_
#define CONNECTIONMANAGER_H_

#include "Component.h"
#include "net/ConvergenceLayer.h"
#include "net/BundleReceiver.h"
#include "core/EventReceiver.h"
#include <ibrdtn/data/EID.h>
#include "core/Node.h"
#include <ibrcommon/Exceptions.h>

#include <set>

namespace dtn
{
	namespace net
	{
		class NeighborNotAvailableException : public ibrcommon::Exception
		{
		public:
			NeighborNotAvailableException(string what = "The requested connection is not available.") throw() : ibrcommon::Exception(what)
			{
			};
		};

		class ConnectionNotAvailableException : public ibrcommon::Exception
		{
		public:
			ConnectionNotAvailableException(string what = "The requested connection is not available.") throw() : ibrcommon::Exception(what)
			{
			};
		};

		class ConnectionManager : public dtn::core::EventReceiver, public dtn::daemon::IntegratedComponent
		{
		public:
			ConnectionManager();
			virtual ~ConnectionManager();

			void addConnection(const dtn::core::Node &n);
			void addConvergenceLayer(ConvergenceLayer *cl);

			void queue(const dtn::data::EID &eid, const dtn::data::Bundle &b);

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const dtn::core::Event *evt);

			class ShutdownException : public ibrcommon::Exception
			{
			public:
				ShutdownException(string what = "System shutdown") throw() : ibrcommon::Exception(what)
				{
				};
			};

			void queue(const ConvergenceLayer::Job &job);

			const std::list<dtn::core::Node> getNeighbors();

		protected:
			void discovered(dtn::core::Node &node);
			void check_discovered();

			virtual void componentUp();
			virtual void componentDown();

		private:
			void queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job);

			bool _shutdown;

			ibrcommon::Mutex _node_lock;
			ibrcommon::Mutex _cl_lock;
			std::set<ConvergenceLayer*> _cl;
			std::set<dtn::core::Node> _static_connections;
			std::list<dtn::core::Node> _discovered_nodes;
		};
	}
}

#endif /* CONNECTIONMANAGER_H_ */
