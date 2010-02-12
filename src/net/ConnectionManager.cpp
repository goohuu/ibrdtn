/*
 * ConnectionManager.cpp
 *
 *  Created on: 24.09.2009
 *      Author: morgenro
 */

#include "net/ConnectionManager.h"
#include "net/UDPConvergenceLayer.h"
#include "net/TCPConvergenceLayer.h"
#include "core/EventSwitch.h"
#include "core/NodeEvent.h"
#include "core/RouteEvent.h"
#include "core/BundleEvent.h"
#include "ibrcommon/net/tcpserver.h"
#include "ibrcommon/TimeMeasurement.h"
#include "ibrcommon/Math.h"
#include "core/BundleCore.h"

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		ConnectionManager::ConnectionManager(int concurrent_transmitter)
		 : _shutdown(false)
		{
			// create transmitter
			for (int i = 0; i < concurrent_transmitter; i++)
			{
				ConnectionManager::Transmitter *t = new ConnectionManager::Transmitter(*this);
				t->start();
				_transmitter_list.push_back(t);
			}
		}

		ConnectionManager::~ConnectionManager()
		{
			{
				ibrcommon::MutexLock l(_job_cond);
				_shutdown = true;
				_job_cond.signal(true);
			}

			// delete all transmitter
			for (std::list<ConnectionManager::Transmitter*>::iterator iter = _transmitter_list.begin(); iter != _transmitter_list.end(); iter++)
			{
				(*iter)->waitFor();
				delete (*iter);
			}

			list<ConvergenceLayer*>::iterator iter = _cl.begin();

			while (iter != _cl.end())
			{
				delete (*iter);
				iter++;
			}
		}

		void ConnectionManager::addConnection(const dtn::core::Node &n)
		{
			dtn::core::EventSwitch::raiseEvent(new dtn::core::NodeEvent( n, dtn::core::NODE_AVAILABLE) );
			_static_connections.push_back(n);
		}

		void ConnectionManager::addConvergenceLayer(ConvergenceLayer *cl)
		{
			cl->setBundleReceiver(this);
			_cl.push_back( cl );
		}

		void ConnectionManager::discovered(dtn::core::Node &node)
		{
			// ignore messages of ourself
			if (EID(node.getURI()) == dtn::core::BundleCore::local) return;

			// search for the dtn node
			list<Node>::iterator iter = _discovered_nodes.begin();
			Node n(PERMANENT);

			while (iter != _discovered_nodes.end())
			{
				n = (*iter);
				if ( n.equals(node) )
				{
					// save the node
					(*iter) = node;
					return;
				}

				iter++;
			}

			// not in list, add it.
			_discovered_nodes.push_back( node );

			// announce the new node
			EventSwitch::raiseEvent(new NodeEvent(node, dtn::core::NODE_AVAILABLE));
		}

		void ConnectionManager::check_discovered()
		{
			// search for outdated nodes
			list<Node>::iterator iter = _discovered_nodes.begin();
			Node n(PERMANENT);

			while (iter != _discovered_nodes.end())
			{
				n = (*iter);

				if ( !n.decrementTimeout(1) )
				{
					// node is outdated -> remove it
					list<Node>::iterator eraseme = iter;
					iter++;
					_discovered_nodes.erase( eraseme );

					// announce the node unavailable event
					EventSwitch::raiseEvent(new NodeEvent(n, dtn::core::NODE_UNAVAILABLE));

					continue;
				}

				(*iter) = n;

				iter++;
			}
		}

		BundleConnection* ConnectionManager::getConnection(const dtn::core::Node &node)
		{
			if ((node.getProtocol() == UNDEFINED) || (node.getProtocol() == UNSUPPORTED))
				throw ConnectionNotAvailableException();

			for (list<ConvergenceLayer*>::const_iterator iter = _cl.begin(); iter != _cl.end(); iter++)
			{
				try {
					BundleConnection *conn = (*iter)->getConnection(node);

					if (conn != NULL)
					{
						return conn;
					}
				} catch (ibrcommon::tcpserver::SocketException ex) {

				}
			}

			throw ConnectionNotAvailableException();
		}

		void ConnectionManager::received(const dtn::data::EID &eid, const Bundle &b)
		{
			// raise default bundle received event
			EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_RECEIVED) );
			EventSwitch::raiseEvent( new RouteEvent(b, ROUTE_PROCESS_BUNDLE) );
		}

		void ConnectionManager::send(const dtn::data::EID &eid, const dtn::data::Bundle &b)
		{
			ibrcommon::MutexLock l(_job_cond);
			_job_queue.push( Job(eid, b) );
			_job_cond.signal(false);
		}

		void ConnectionManager::send(const ConnectionManager::Job &job)
		{
			BundleConnection *conn = getConnection(job._destination);

//			if (conn->isBusy())
//			{
//#ifdef DO_DEBUG_OUTPUT
//				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "requeue bundle cause of busy connection" << endl;
//#endif
//				EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_RECEIVED) );
//				EventSwitch::raiseEvent( new RouteEvent(b, ROUTE_PROCESS_BUNDLE) );
//				return;
//			}

			ibrcommon::TimeMeasurement m;
			m.start();

			try {
				conn->write(job._bundle);
				m.stop();
				// get throughput
				double kbytes_per_second = (job._bundle.getSize() / m.getSeconds()) / 1024;

				// print out throughput
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "transfer completed after " << m << " with " << ibrcommon::Math::Round(kbytes_per_second, 2) << " kb/s" << endl;
			} catch (BundleConnection::ConnectionInterruptedException ex) {
				m.stop();
				// get throughput
				double kbytes_per_second = (ex.getLastAck() / m.getSeconds()) / 1024;

				// print out throughput
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "transfer interrupted after " << m << " with " << ibrcommon::Math::Round(kbytes_per_second, 2) << " kb/s" << endl;

				// TODO: the connection has been interrupted => create a fragment

			} catch (dtn::exceptions::IOException ex) {
				m.stop();
				// the connection has been terminated and fragmentation is not possible => requeue the bundle

#ifdef DO_DEBUG_OUTPUT
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "fragmentation is not possible => requeue the bundle" << endl;
				ibrcommon::slog << ibrcommon::SYSLOG_DEBUG << "Exception: " << ex.what() << endl;
#endif
				EventSwitch::raiseEvent( new BundleEvent(job._bundle, BUNDLE_RECEIVED) );
				EventSwitch::raiseEvent( new RouteEvent(job._bundle, ROUTE_PROCESS_BUNDLE) );
			}
		}

		BundleConnection* ConnectionManager::getConnection(const dtn::data::EID &eid)
		{
			// seek for a connection in the static list
			list<dtn::core::Node>::const_iterator iter = _static_connections.begin();
			while (iter != _static_connections.end())
			{
				const dtn::core::Node &n = (*iter);
				if (n.getURI() == eid.getNodeEID())
				{
					// create a connection
					return getConnection(n);
				}

				iter++;
			}

			// seek for a connection in the discovery list
			iter = _discovered_nodes.begin();
			while (iter != _discovered_nodes.end())
			{
				const dtn::core::Node &n = (*iter);
				if (n.getURI() == eid.getNodeEID())
				{
					// create a connection
					return getConnection(n);
				}

				iter++;
			}

			throw NeighborNotAvailableException("No active connection to this neighbor available!");
		}

		ConnectionManager::Job ConnectionManager::getJob()
		{
			ibrcommon::MutexLock l(_job_cond);

			while (_job_queue.empty())
			{
				_job_cond.wait();
				if (_shutdown) throw ShutdownException();
			}

			ConnectionManager::Job ret = _job_queue.front();
			_job_queue.pop();
			return ret;
		}

		ConnectionManager::Transmitter::Transmitter(ConnectionManager &manager)
		 : _running(true), _manager(manager)
		{
		}

		ConnectionManager::Transmitter::~Transmitter()
		{
			// TODO: abort current jobs
			_running = false;
			join();
		}

		void ConnectionManager::Transmitter::run()
		{
			try {
				while (_running)
				{
					ConnectionManager::Job j = _manager.getJob();
					_manager.send(j);
					yield();
				}
			} catch (ConnectionManager::ShutdownException ex) {

			}
		}

		ConnectionManager::Job::Job(const dtn::data::EID &eid, const dtn::data::Bundle &b)
		 : _bundle(b), _destination(eid)
		{
		}

		ConnectionManager::Job::~Job()
		{
		}
	}
}
