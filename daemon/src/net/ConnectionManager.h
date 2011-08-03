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
			void removeConnection(const dtn::core::Node &n);

			void addConvergenceLayer(ConvergenceLayer *cl);

			void queue(const dtn::data::EID &eid, const dtn::data::BundleID &b);

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

			void open(const dtn::core::Node &node);

			void queue(const ConvergenceLayer::Job &job);

			/**
			 * get a set with all neighbors
			 * @return
			 */
			const std::set<dtn::core::Node> getNeighbors();

			/**
			 * Checks if a node is already known as neighbor.
			 * @param
			 * @return
			 */
			bool isNeighbor(const dtn::core::Node&);

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

		protected:
			/**
			 * trigger for periodical discovery of nodes
			 * @param node
			 */
			void discovered(const dtn::core::Node &node);

			/**
			 * checks for timed out nodes
			 */
			void check_discovered();

			virtual void componentUp();
			virtual void componentDown();

		private:
			/**
			 *  queue a bundle for delivery
			 */
			void queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job);

			// if set to true, this module will shutdown
			bool _shutdown;

			// mutex for the list of convergence layers
			ibrcommon::Mutex _cl_lock;

			// contains all configured convergence layers
			std::set<ConvergenceLayer*> _cl;

			// mutex for the lists of nodes
			ibrcommon::Mutex _node_lock;

			// contains all static configured nodes
			std::set<dtn::core::Node> _static_nodes;

			// contains all dynamic discovered nodes
			std::set<dtn::core::Node> _discovered_nodes;

			// contains all connected nodes
			std::set<dtn::core::Node> _connected_nodes;
		};
	}
}

#endif /* CONNECTIONMANAGER_H_ */
