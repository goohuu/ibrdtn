/*
 * ConnectionManager.h
 *
 *  Created on: 18.09.2009
 *      Author: morgenro
 */

#ifndef CONNECTIONMANAGER_H_
#define CONNECTIONMANAGER_H_

#include "ibrdtn/config.h"
#include "net/BundleConnection.h"
#include "net/ConvergenceLayer.h"
#include "net/BundleReceiver.h"
#include "core/EventReceiver.h"
#include "ibrdtn/data/EID.h"
#include "core/Node.h"
#include "ibrcommon/Exceptions.h"

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

		class ConnectionManager : public dtn::net::BundleReceiver, public dtn::core::EventReceiver
		{
		public:
			ConnectionManager(int concurrent_transmitter = 1);
			virtual ~ConnectionManager();

			void addConnection(const dtn::core::Node &n);
			void addConvergenceLayer(ConvergenceLayer *cl);

			void received(const dtn::data::EID &eid, const dtn::data::Bundle &b);
			void send(const dtn::data::EID &eid, const dtn::data::Bundle &b);

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

			map<dtn::data::EID, BundleConnection*> _active_connections;
			ibrcommon::Mutex _active_connections_lock;
		};
	}
}

#endif /* CONNECTIONMANAGER_H_ */
