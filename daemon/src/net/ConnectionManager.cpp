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

#include "core/NodeEvent.h"
#include "core/TimeEvent.h"

#include <iostream>
#include <iomanip>

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		ConnectionManager::ConnectionManager(int concurrent_transmitter)
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
		}

		void ConnectionManager::addConnection(const dtn::core::Node &n)
		{
			{
				ibrcommon::MutexLock l(_node_lock);
				_static_connections.insert(n);
			}

			dtn::core::NodeEvent::raise(n, dtn::core::NODE_AVAILABLE);
		}

		void ConnectionManager::addConvergenceLayer(ConvergenceLayer *cl)
		{
			ibrcommon::MutexLock l(_cl_lock);
			cl->setBundleReceiver(this);
			_cl.insert( cl );
		}

		void ConnectionManager::discovered(dtn::core::Node &node)
		{
			{
				ibrcommon::MutexLock l(_node_lock);

				// ignore messages of ourself
				if (EID(node.getURI()) == dtn::core::BundleCore::local) return;

				std::list<dtn::core::Node>::iterator iter = _discovered_nodes.begin();
				while (iter != _discovered_nodes.end())
				{
					if ((*iter) == node)
					{
						(*iter) = node;
						return;
					}
					iter++;
				}

				_discovered_nodes.push_back(node);
			}

			// announce the new node
			dtn::core::NodeEvent::raise(node, dtn::core::NODE_AVAILABLE);
		}

		void ConnectionManager::check_discovered()
		{
			Node n;

			{
				ibrcommon::MutexLock l(_node_lock);

				// search for outdated nodes
				std::list<Node>::iterator iter = _discovered_nodes.begin();

				while (iter != _discovered_nodes.end())
				{
					if ( !(*iter).decrementTimeout(1) )
					{
						n = (*iter);

						// node is outdated -> remove it
						_discovered_nodes.erase( iter++ );
						break;
					}
					else
					{
						iter++;
					}
				}
			}

			if (n.getAddress() != "dtn:unknown")
			{
				// announce the node unavailable event
				dtn::core::NodeEvent::raise(n, dtn::core::NODE_UNAVAILABLE);
			}
		}

		void ConnectionManager::received(const dtn::data::EID &eid, const Bundle &b)
		{
			// raise default bundle received event
			dtn::net::BundleReceivedEvent::raise(eid, b);
		}

		void ConnectionManager::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			if ((node.getProtocol() == UNDEFINED) || (node.getProtocol() == UNSUPPORTED))
				throw ConnectionNotAvailableException();

			ibrcommon::MutexLock l(_cl_lock);

			// search for the right cl
			for (std::set<ConvergenceLayer*>::iterator iter = _cl.begin(); iter != _cl.end(); iter++)
			{
				ConvergenceLayer *cl = (*iter);
				if (node.getProtocol() == cl->getDiscoveryProtocol())
				{
					cl->queue(node, job);
				}
			}
		}

		void ConnectionManager::queue(const ConvergenceLayer::Job &job)
		{
			ibrcommon::MutexLock l(_node_lock);

			// seek for a connection in the static list
			for (std::set<dtn::core::Node>::const_iterator iter = _static_connections.begin(); iter != _static_connections.end(); iter++)
			{
				const dtn::core::Node &n = (*iter);
				if (EID(n.getURI()) == job._destination)
				{
					// queue at the convergence layer
					queue(n, job);
					return;
				}
			}

			// seek for a connection in the discovery list
			for (std::list<dtn::core::Node>::iterator iter = _discovered_nodes.begin(); iter != _discovered_nodes.end(); iter++)
			{
				const dtn::core::Node &n = (*iter);
				if (EID(n.getURI()) == job._destination)
				{
					// queue at the convergence layer
					queue(n, job);
					return;
				}
			}

			throw NeighborNotAvailableException("No active connection to this neighbor available!");
		}

		void ConnectionManager::queue(const dtn::data::EID &eid, const dtn::data::Bundle &b)
		{
			queue( ConvergenceLayer::Job(eid, b) );
		}

		const std::list<dtn::core::Node> ConnectionManager::getNeighbors()
		{
			ibrcommon::MutexLock l(_node_lock);
			std::list<dtn::core::Node> _nodes;

			for (std::set<dtn::core::Node>::const_iterator iter = _static_connections.begin(); iter != _static_connections.end(); iter++)
			{
				_nodes.push_back( *iter );
			}

			for (std::list<dtn::core::Node>::const_iterator iter = _discovered_nodes.begin(); iter != _discovered_nodes.end(); iter++)
			{
				_nodes.push_back( *iter );
			}

			return _nodes;
		}
	}
}
