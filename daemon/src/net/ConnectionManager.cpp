/*
 * ConnectionManager.cpp
 *
 *  Created on: 24.09.2009
 *      Author: morgenro
 */

#include "net/ConnectionManager.h"
#include "net/UDPConvergenceLayer.h"
#include "net/TCPConvergenceLayer.h"
#include "net/BundleReceivedEvent.h"
#include "core/NodeEvent.h"
#include "core/BundleEvent.h"
#include "ibrcommon/net/tcpserver.h"
#include "core/BundleCore.h"
#include "routing/RequeueBundleEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/ConnectionEvent.h"

#include "core/NodeEvent.h"
#include "core/TimeEvent.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <typeinfo>
#include <ibrcommon/Logger.h>

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		struct CompareNodeDestination:
		public std::binary_function< dtn::core::Node, dtn::data::EID, bool > {
			bool operator() ( const dtn::core::Node &node, const dtn::data::EID &destination ) const {
				return dtn::data::EID(node.getURI()) == destination;
			}
		};

		ConnectionManager::ConnectionManager()
		 : _shutdown(false)
		{
		}

		ConnectionManager::~ConnectionManager()
		{
		}

		void ConnectionManager::componentUp()
		{
			bindEvent(TimeEvent::className);
			bindEvent(NodeEvent::className);
			bindEvent(ConnectionEvent::className);
		}

		void ConnectionManager::componentDown()
		{
			{
				ibrcommon::MutexLock l(_cl_lock);
				// clear the list of convergence layers
				_cl.clear();
			}

			unbindEvent(NodeEvent::className);
			unbindEvent(TimeEvent::className);
			unbindEvent(ConnectionEvent::className);
		}

		void ConnectionManager::raiseEvent(const dtn::core::Event *evt)
		{
			const NodeEvent *nodeevent = dynamic_cast<const NodeEvent*>(evt);
			const TimeEvent *timeevent = dynamic_cast<const TimeEvent*>(evt);

			if (timeevent != NULL)
			{
				if (timeevent->getAction() == TIME_SECOND_TICK)
				{
					check_discovered();
				}
			}
			else if (nodeevent != NULL)
			{
				Node n = nodeevent->getNode();

				if (nodeevent->getAction() == NODE_INFO_UPDATED)
				{
					discovered(n);
				}
			}

			try {
				const ConnectionEvent &connection = dynamic_cast<const ConnectionEvent&>(*evt);

				switch (connection.state)
				{
					case ConnectionEvent::CONNECTION_UP:
					{
						// if this node was unknown before...
						if (!isNeighbor(connection.node))
						{
							// announce the new node
							dtn::core::NodeEvent::raise(connection.node, dtn::core::NODE_AVAILABLE);
						}

						ibrcommon::MutexLock l(_node_lock);
						_connected_nodes.insert(connection.node);
						break;
					}

					case ConnectionEvent::CONNECTION_DOWN:
					{
						{
							ibrcommon::MutexLock l(_node_lock);
							_connected_nodes.erase(connection.node);
						}

						// if this node is gone...
						if (!isNeighbor(connection.node))
						{
							// announce the unavailable event
							dtn::core::NodeEvent::raise(connection.node, dtn::core::NODE_UNAVAILABLE);
						}
						break;
					}

					default:
						break;
				}

			} catch (std::bad_cast) {

			}
		}

		void ConnectionManager::addConnection(const dtn::core::Node &n)
		{
			{
				ibrcommon::MutexLock l(_node_lock);
				_static_nodes.insert(n);
			}

			dtn::core::NodeEvent::raise(n, dtn::core::NODE_AVAILABLE);
		}

		void ConnectionManager::addConvergenceLayer(ConvergenceLayer *cl)
		{
			ibrcommon::MutexLock l(_cl_lock);
			_cl.insert( cl );
		}

		void ConnectionManager::discovered(dtn::core::Node &node)
		{
			// ignore messages of ourself
			if (EID(node.getURI()) == dtn::core::BundleCore::local) return;

			// if this node was unknown before...
			if (!isNeighbor(node))
			{
				// announce the new node
				dtn::core::NodeEvent::raise(node, dtn::core::NODE_AVAILABLE);
			}

			ibrcommon::MutexLock l(_node_lock);
			_discovered_nodes.erase(node);
			_discovered_nodes.insert(node);
		}

		void ConnectionManager::check_discovered()
		{
			std::list<dtn::core::Node> unavailables;

			{
				ibrcommon::MutexLock l(_node_lock);

				// search for outdated nodes
				std::set<dtn::core::Node>::iterator iter = _discovered_nodes.begin();

				while (iter != _discovered_nodes.end())
				{
					dtn::core::Node n = (*iter);

					// node is outdated -> remove it
					_discovered_nodes.erase( iter++ );

					if ( !n.decrementTimeout(1) )
					{
						unavailables.push_back(n);
					}
					else
					{
						_discovered_nodes.insert(n);
					}
				}
			}

			for(std::list<dtn::core::Node>::const_iterator iter = unavailables.begin(); iter != unavailables.end(); iter++)
			{
				// if this node is gone...
				if (!isNeighbor(*iter))
				{
					// announce the unavailable event
					dtn::core::NodeEvent::raise(*iter, dtn::core::NODE_UNAVAILABLE);
				}
			}
		}

		void ConnectionManager::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			if ((node.getProtocol() == Node::CONN_UNDEFINED) || (node.getProtocol() == Node::CONN_UNSUPPORTED))
				throw ConnectionNotAvailableException();

			ibrcommon::MutexLock l(_cl_lock);

			// search for the right cl
			for (std::set<ConvergenceLayer*>::iterator iter = _cl.begin(); iter != _cl.end(); iter++)
			{
				ConvergenceLayer *cl = (*iter);
				if (node.getProtocol() == cl->getDiscoveryProtocol())
				{
					cl->queue(node, job);

					// stop here, we queued the bundle already
					return;
				}
			}
		}

		void ConnectionManager::queue(const ConvergenceLayer::Job &job)
		{
			ibrcommon::MutexLock l(_node_lock);

			// create a match rank list
			std::list<dtn::core::Node> match_rank;

			// we prefer TCP as primary connection type
			match_rank.push_back(dtn::core::Node(job._destination, Node::CONN_TCPIP));

			// use UDP as seconds connection type
			match_rank.push_back(dtn::core::Node(job._destination, Node::CONN_UDPIP));

			// use Bluetooth as fourth connection type
			match_rank.push_back(dtn::core::Node(job._destination, Node::CONN_BLUETOOTH));

			// use ZigBee as third connection type
			match_rank.push_back(dtn::core::Node(job._destination, Node::CONN_ZIGBEE));

			if (IBRCOMMON_LOGGER_LEVEL >= 50)
			{
				IBRCOMMON_LOGGER_DEBUG(50) << "## static node list ##" << IBRCOMMON_LOGGER_ENDL;
				for (std::set<dtn::core::Node>::const_iterator iter = _static_nodes.begin(); iter != _static_nodes.end(); iter++)
				{
					const dtn::core::Node &n = (*iter);
					IBRCOMMON_LOGGER_DEBUG(50) << n.toString() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			if (IBRCOMMON_LOGGER_LEVEL >= 50)
			{
				IBRCOMMON_LOGGER_DEBUG(50) << "## dynamic node list ##" << IBRCOMMON_LOGGER_ENDL;
				for (std::set<dtn::core::Node>::const_iterator iter = _discovered_nodes.begin(); iter != _discovered_nodes.end(); iter++)
				{
					const dtn::core::Node &n = (*iter);
					IBRCOMMON_LOGGER_DEBUG(50) << n.toString() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			if (IBRCOMMON_LOGGER_LEVEL >= 50)
			{
				IBRCOMMON_LOGGER_DEBUG(50) << "## connected node list ##" << IBRCOMMON_LOGGER_ENDL;
				for (std::set<dtn::core::Node>::const_iterator iter = _connected_nodes.begin(); iter != _connected_nodes.end(); iter++)
				{
					const dtn::core::Node &n = (*iter);
					IBRCOMMON_LOGGER_DEBUG(50) << n.toString() << IBRCOMMON_LOGGER_ENDL;
				}
			}

			// iterate through all matches in the rank list
			for (std::list<dtn::core::Node>::const_iterator imatch = match_rank.begin(); imatch != match_rank.end(); imatch++)
			{
				const dtn::core::Node &match = (*imatch);
				IBRCOMMON_LOGGER_DEBUG(50) << "match for " << match.toString() << IBRCOMMON_LOGGER_ENDL;

				try {
					// queue to a static node
					std::set<dtn::core::Node>::const_iterator iter = _static_nodes.find(match);

					if (iter != _static_nodes.end())
					{
						const dtn::core::Node &next = (*iter);
						IBRCOMMON_LOGGER_DEBUG(50) << "next hop: " << next.toString() << IBRCOMMON_LOGGER_ENDL;

						queue(next, job);
						return;
					}
				} catch (...) {

				}

				try {
					// queue to a dynamic discovered node
					std::set<dtn::core::Node>::const_iterator iter = _discovered_nodes.find(match);

					if (iter != _discovered_nodes.end())
					{
						const dtn::core::Node &next = (*iter);
						IBRCOMMON_LOGGER_DEBUG(50) << "next hop: " << next.toString() << IBRCOMMON_LOGGER_ENDL;

						queue(next, job);
						return;
					}
				} catch (...) {

				}

				try {
					// queue to a connected node
					std::set<dtn::core::Node>::const_iterator iter = _connected_nodes.find(match);

					if (iter != _connected_nodes.end())
					{
						const dtn::core::Node &next = (*iter);
						IBRCOMMON_LOGGER_DEBUG(50) << "next hop: " << next.toString() << IBRCOMMON_LOGGER_ENDL;

						queue(next, job);
						return;
					}
				} catch (...) {

				}
			}

			throw NeighborNotAvailableException("No active connection to this neighbor available!");
		}

		void ConnectionManager::queue(const dtn::data::EID &eid, const dtn::data::Bundle &b)
		{
			queue( ConvergenceLayer::Job(eid, b) );
		}

		const std::set<dtn::core::Node> ConnectionManager::getNeighbors()
		{
			ibrcommon::MutexLock l(_node_lock);

			std::set<dtn::core::Node> _nodes;

			for (std::set<dtn::core::Node>::const_iterator iter = _static_nodes.begin(); iter != _static_nodes.end(); iter++)
			{
				_nodes.insert( *iter );
			}

			for (std::set<dtn::core::Node>::const_iterator iter = _discovered_nodes.begin(); iter != _discovered_nodes.end(); iter++)
			{
				_nodes.insert( *iter );
			}

			for (std::set<dtn::core::Node>::const_iterator iter = _connected_nodes.begin(); iter != _connected_nodes.end(); iter++)
			{
				_nodes.insert( *iter );
			}

			return _nodes;
		}


		bool ConnectionManager::isNeighbor(const dtn::core::Node &node)
		{
			ibrcommon::MutexLock l(_node_lock);

			// search for the node in all sets
			if (_static_nodes.find(node) != _static_nodes.end())
			{
				return true;
			}

			if (_discovered_nodes.find(node) != _discovered_nodes.end())
			{
				return true;
			}

			if (_connected_nodes.find(node) != _connected_nodes.end())
			{
				return true;
			}

			return false;
		}
	}
}
