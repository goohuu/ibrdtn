#include "emma/EmmaConvergenceLayer.h"
#include "emma/DiscoverBlock.h"
#include "ibrdtn/utils/Utils.h"
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

	EmmaConvergenceLayer::EmmaConvergenceLayer(EID eid, string bind_addr, unsigned short port, string broadcast, unsigned int mtu)
		: m_eid(eid), m_bindaddr(bind_addr), m_bindport(port), m_direct_cl(NULL), m_broadcast_cl(NULL)
	{
		// register at event switch
		EventSwitch::registerEventReceiver(PositionEvent::className, this);
		EventSwitch::registerEventReceiver(TimeEvent::className, this);

		m_direct_cl = new UDPConvergenceLayer(bind_addr, port, mtu);
		m_broadcast_cl = new UDPConvergenceLayer(broadcast, port, true, mtu);

		m_direct_cl->setBundleReceiver(this);
		m_broadcast_cl->setBundleReceiver(this);

		start();
	}

	EmmaConvergenceLayer::~EmmaConvergenceLayer()
	{
		EventSwitch::unregisterEventReceiver(PositionEvent::className, this);
		EventSwitch::unregisterEventReceiver(TimeEvent::className, this);

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
		dtn::utils::MutexLock l(m_receivelock);

		// check for discovery blocks to process
		list<Block*> blocks = b.getBlocks(DiscoverBlock::BLOCK_TYPE);

		if (!blocks.empty())
		{
			// ignore own bundles
			if ( b._source.sameHost(m_eid) ) return;

			DiscoverBlock *discover = dynamic_cast<DiscoverBlock*>( blocks.front() );

			if (discover != NULL)
			{
				// create a node and announce it to the router
				// max. 5 seconds round trip time
				Node n(FLOATING, 5);
				n.setAddress( discover->_address );
				n.setPort( discover->_port );
				n.setURI( b._source.getString() );
				n.setTimeout( 5 ); // 5 seconds timeout
				n.setConvergenceLayer(this);

				// create a event
				EventSwitch::raiseEvent(new NodeEvent(n, dtn::core::NODE_INFO_UPDATED));

				if ( b._destination == "dtn:discovery" ) return;
			}
		}

		ConvergenceLayer::eventBundleReceived(b);
	}

	void EmmaConvergenceLayer::start()
	{
		m_direct_cl->start();
		m_broadcast_cl->start();
	}

	void EmmaConvergenceLayer::yell()
	{
		/*
		 * send a discovery bundle with broadcast to neighboring nodes
		 */

		// create a discovery bundle
		Bundle bundle;
		DiscoverBlock *block = new DiscoverBlock();

		// set address and port
		block->_address = m_bindaddr;
		block->_port = m_bindport;

		// set the source eid
		bundle._source = m_eid;

		// set the destination eid
		bundle._destination = EID("dtn:discovery");

		// avoid storing & forwarding
		bundle._lifetime = 0;

		// priority expedited
		if (!(bundle._procflags & Bundle::PRIORITY_BIT1)) bundle._procflags += Bundle::PRIORITY_BIT1;
		if (!(bundle._procflags & Bundle::PRIORITY_BIT2)) bundle._procflags += Bundle::PRIORITY_BIT2;

		// commit the block data
		block->commit();

		// append the discover block
		bundle.addBlock(block);

		transmit( bundle );
	}

	void EmmaConvergenceLayer::yell(Node node)
	{
		/*
		 * send a discovery bundle with broadcast to neighboring nodes
		 */

		// create a discovery bundle
		Bundle bundle;
		DiscoverBlock *block = new DiscoverBlock();

		// set address and port
		block->_address = m_bindaddr;
		block->_port = m_bindport;

		// set the source eid
		bundle._source = m_eid;

		// set the destination eid
		bundle._destination = EID( node.getURI() );

		// avoid storing & forwarding
		bundle._lifetime = 0;

		// priority expedited
		if (!(bundle._procflags & Bundle::PRIORITY_BIT1)) bundle._procflags += Bundle::PRIORITY_BIT1;
		if (!(bundle._procflags & Bundle::PRIORITY_BIT2)) bundle._procflags += Bundle::PRIORITY_BIT2;

		// commit the block data
		block->commit();

		// append the discover block
		bundle.addBlock(block);

		transmit( bundle );
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
