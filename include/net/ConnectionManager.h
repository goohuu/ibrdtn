/*
 * ConnectionManager.h
 *
 *  Created on: 18.09.2009
 *      Author: morgenro
 */

#ifndef CONNECTIONMANAGER_H_
#define CONNECTIONMANAGER_H_

#include "ibrdtn/config.h"
#include "ibrdtn/data/Exceptions.h"
#include "net/BundleConnection.h"
#include "net/ConvergenceLayer.h"
#include "net/BundleReceiver.h"
#include "ibrdtn/data/EID.h"
#include "core/Node.h"

namespace dtn
{
	namespace net
	{
		class NeighborNotAvailableException : public dtn::exceptions::Exception
		{
		public:
			NeighborNotAvailableException(string what = "The requested connection is not available.") throw() : Exception(what)
			{
			};
		};

		class ConnectionNotAvailableException : public dtn::exceptions::Exception
		{
		public:
			ConnectionNotAvailableException(string what = "The requested connection is not available.") throw() : Exception(what)
			{
			};
		};

		class ConnectionManager : public dtn::net::BundleReceiver
		{
		public:
			ConnectionManager(int concurrent_transmitter = 1);
			virtual ~ConnectionManager();

			void addConnection(const dtn::core::Node &n);
			void addConvergenceLayer(ConvergenceLayer *cl);

			void received(const dtn::data::EID &eid, const dtn::data::Bundle &b);
			void send(const dtn::data::EID &eid, const dtn::data::Bundle &b);

			class ShutdownException : public dtn::exceptions::Exception
			{
			public:
				ShutdownException(string what = "System shutdown") throw() : Exception(what)
				{
				};
			};

		protected:
			void discovered(dtn::core::Node &node);
			void check_discovered();

		private:
			class Transmitter : public ibrcommon::JoinableThread
			{
			public:
				Transmitter(ConnectionManager &manager);
				virtual ~Transmitter();

				void run();

			private:
				bool _running;
				ConnectionManager &_manager;
			};

			class Job
			{
			public:
				Job(const dtn::data::EID &eid, const dtn::data::Bundle &b);
				~Job();

				dtn::data::Bundle _bundle;
				dtn::data::EID _destination;
			};

			bool _shutdown;

			ConnectionManager::Job getJob();
			void send(const ConnectionManager::Job &job);

			BundleConnection* getConnection(const dtn::data::EID &eid);
			BundleConnection* getConnection(const dtn::core::Node &node);

			std::list<Transmitter*> _transmitter_list;

			std::queue<ConnectionManager::Job> _job_queue;
			ibrcommon::Conditional _job_cond;

			list<ConvergenceLayer*> _cl;
			list<dtn::core::Node> _static_connections;
			list<dtn::core::Node> _discovered_nodes;
		};
	}
}

#endif /* CONNECTIONMANAGER_H_ */
