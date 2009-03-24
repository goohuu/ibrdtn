#include "emma/EmmaConvergenceLayer.h"
#include "emma/DiscoverBlock.h"
#include "utils/Utils.h"
#include "core/EventSwitch.h"
#include "core/NodeEvent.h"
#include "core/TimeEvent.h"

#include "emma/PositionEvent.h"

#include <sys/socket.h>
#include <errno.h>
#include <iostream>

using namespace dtn::core;
using namespace dtn::data;
using namespace dtn::utils;

namespace emma
{
	const int EmmaConvergenceLayer::MAX_SIZE = 1024;

	EmmaConvergenceLayer::EmmaConvergenceLayer(string eid, string bind_addr, unsigned short port, string broadcast, unsigned int mtu)
		: m_eid(eid), m_bindaddr(bind_addr), m_bindport(port), m_direct_cl(NULL), m_broadcast_cl(NULL)
	{
		// register at event switch
		EventSwitch::registerEventReceiver(PositionEvent::className, this);
		EventSwitch::registerEventReceiver(TimeEvent::className, this);

		// register DiscoverBlock structure at the bundle factory
		m_dfactory = new DiscoverBlockFactory();
		BundleFactory::getInstance().registerExtensionBlock(m_dfactory);

		m_direct_cl = new UDPConvergenceLayer(bind_addr, port, mtu);
		m_broadcast_cl = new UDPConvergenceLayer(broadcast, port, true, mtu);

		m_direct_cl->setBundleReceiver(this);
		m_broadcast_cl->setBundleReceiver(this);

		initialize();
	}

	EmmaConvergenceLayer::~EmmaConvergenceLayer()
	{
		terminate();

		EventSwitch::unregisterEventReceiver(PositionEvent::className, this);
		EventSwitch::unregisterEventReceiver(TimeEvent::className, this);

		BundleFactory::getInstance().unregisterExtensionBlock(m_dfactory);
		delete m_dfactory;

		if (m_broadcast_cl != m_direct_cl)
		{
			delete m_broadcast_cl;
		}

		delete m_direct_cl;
	}

	TransmitReport EmmaConvergenceLayer::transmit(const Bundle &b)
	{
		return m_broadcast_cl->transmit(b);
	}

	TransmitReport EmmaConvergenceLayer::transmit(const Bundle &b, const Node &node)
	{
		return m_direct_cl->transmit(b, node);
	}

	void EmmaConvergenceLayer::received(const ConvergenceLayer &cl, const Bundle &b)
	{
		MutexLock l(m_receivelock);

		// check for discovery blocks to process
		list<Block*> blocks = b.getBlocks(DiscoverBlock::BLOCK_TYPE);

		if (!blocks.empty())
		{
			DiscoverBlock *discover = dynamic_cast<DiscoverBlock*>( blocks.front() );

			if ( b.getSource() == m_eid ) return;

			if ( discover != NULL)
			{
				// create a node and announce it to the router
				Node n(FLOATING);
				n.setAddress( discover->getConnectionAddress() );
				n.setPort( discover->getConnectionPort() );
				n.setURI( b.getSource() );
				n.setTimeout( 5 ); // 5 seconds timeout
				n.setConvergenceLayer(this);

				if (discover->getOptionals() > 1)
				{
					n.setPosition( make_pair( discover->getLatitude(), discover->getLongitude() ) );
				}

				// create a event
				EventSwitch::raiseEvent(new NodeEvent(n, dtn::core::NODE_INFO_UPDATED));

				if ( b.getDestination() == "dtn:discovery" ) return;
			}
		}

		ConvergenceLayer::eventBundleReceived(b);
	}

	void EmmaConvergenceLayer::initialize()
	{
		m_direct_cl->start();
		m_broadcast_cl->start();
	}

	void EmmaConvergenceLayer::terminate()
	{
		m_direct_cl->abort();
		m_broadcast_cl->abort();
	}

	void EmmaConvergenceLayer::yell()
	{
		/*
		 * send a discovery bundle with broadcast to neighboring nodes
		 */

		// create a discovery bundle
		BundleFactory &fac = BundleFactory::getInstance();
		Bundle *bundle = fac.newBundle();
		DiscoverBlock *block = DiscoverBlockFactory::newDiscoverBlock();

		// set address and port
		block->setConnectionAddress(m_bindaddr);
		block->setConnectionPort(m_bindport);

		// set the source eid
		bundle->setSource( m_eid );

		// set the destination eid
		bundle->setDestination("dtn:discovery");

		// avoid storing & forwarding
		bundle->setInteger(LIFETIME, 0);

		// priority expedited
		PrimaryFlags flags = bundle->getPrimaryFlags();
		flags.setPriority(EXPEDITED);
		bundle->setPrimaryFlags(flags);

		// add aditional data: GPS position
		block->setLatitude( m_position.first );
		block->setLongitude( m_position.second );

		// append the discover block
		bundle->appendBlock(block);

		transmit( *bundle );

		delete bundle;
	}

	void EmmaConvergenceLayer::yell(Node node)
	{
		/*
		 * send a discovery bundle with broadcast to neighboring nodes
		 */

		// create a discovery bundle
		BundleFactory &fac = BundleFactory::getInstance();
		Bundle *bundle = fac.newBundle();
		DiscoverBlock *block = DiscoverBlockFactory::newDiscoverBlock();

		// set address and port
		block->setConnectionAddress(m_bindaddr);
		block->setConnectionPort(m_bindport);

		bundle->appendBlock(block);

		// set the source eid
		bundle->setSource( m_eid );

		// set the destination eid
		bundle->setDestination( node.getURI() );

		// Avoid storing & forwarding
		bundle->setInteger(LIFETIME, 0);

		// Priority expedited
		PrimaryFlags flags = bundle->getPrimaryFlags();
		flags.setPriority(EXPEDITED);
		bundle->setPrimaryFlags(flags);

		// add aditional data: GPS position
		block->setLatitude( m_position.first );
		block->setLongitude( m_position.second );

		// append the discover block
		bundle->appendBlock(block);

		transmit( *bundle, node );

		delete bundle;
	}

	void EmmaConvergenceLayer::raiseEvent(const Event *evt)
	{
		const PositionEvent *event = dynamic_cast<const PositionEvent*>(evt);
		const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

		if (event != NULL)
		{
			m_position = event->getPosition();
		}
		else if (time != NULL)
		{
			yell();
		}
	}
}
