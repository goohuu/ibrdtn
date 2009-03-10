#include "emma/EmmaConvergenceLayer.h"
#include "emma/DiscoverBlock.h"
#include "utils/Utils.h"
#include "core/EventSwitch.h"
#include "core/NodeEvent.h"

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
		: Service("EmmaConvergenceLayer"), m_eid(eid), m_bindaddr(bind_addr), m_bindport(port), m_lastyell(0), m_direct_cl(NULL), m_broadcast_cl(NULL)
	{
		// register at event switch
		EventSwitch::registerEventReceiver(PositionEvent::className, this);

		// register DiscoverBlock structure at the bundle factory
		m_dfactory = new DiscoverBlockFactory();
		BundleFactory::getInstance().registerExtensionBlock(m_dfactory);

		m_direct_cl = new UDPConvergenceLayer(bind_addr, port, mtu);
		m_broadcast_cl = new UDPConvergenceLayer(broadcast, port, true, mtu);

		m_direct_cl->setBundleReceiver(this);
		m_broadcast_cl->setBundleReceiver(this);
	}

	EmmaConvergenceLayer::~EmmaConvergenceLayer()
	{
		EventSwitch::unregisterEventReceiver(PositionEvent::className, this);

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
		// Verwende Node um das Ziel zu definieren
		return m_broadcast_cl->transmit(b);
	}

	TransmitReport EmmaConvergenceLayer::transmit(const Bundle &b, const Node &node)
	{
		return m_direct_cl->transmit(b, node);
	}

	void EmmaConvergenceLayer::received(const ConvergenceLayer &cl, Bundle &b)
	{
		MutexLock l(m_receivelock);

		// Wenn es ein DiscoverBundle ist dann verarbeite es gleich.
		list<Block*> blocks = b.getBlocks(DiscoverBlock::BLOCK_TYPE);

		DiscoverBlock *discover = dynamic_cast<DiscoverBlock*>( blocks.front() );

		if ( discover != NULL)
		{
			// Erstelle eine Node und leite dieses Objekt an den Router weiter
			Node n(FLOATING);
			n.setAddress( discover->getConnectionAddress() );
			n.setPort( discover->getConnectionPort() );
			n.setURI( b.getSource() );
			n.setTimeout( 5 ); // 5 Sekunden Timeout
			n.setConvergenceLayer(this);

			if (discover->getOptionals() > 1)
			{
				n.setPosition( make_pair( discover->getLatitude(), discover->getLongitude() ) );
			}

			// create a event
			EventSwitch::raiseEvent(new NodeEvent(n, dtn::core::NODE_INFO_UPDATED));

//			// Zurück rufen, wenn dies ein Broadcast war und nicht von uns selbst stammt
//			if ( ( b->getDestination() == "dtn:discovery" ) && ( b->getSource() != m_eid ) )
//			{
//				yell( n );
//			}
		}

		ConvergenceLayer::eventBundleReceived(b);
	}

	void EmmaConvergenceLayer::initialize()
	{
		// ConvergenceLayer starten
		m_direct_cl->start();
		m_broadcast_cl->start();
	}

	void EmmaConvergenceLayer::terminate()
	{
		// ConvergenceLayer stoppen
		m_direct_cl->abort();
		m_broadcast_cl->abort();
	}

	void EmmaConvergenceLayer::tick()
	{
		// Jede Sekunde wollen wir ein yell() ausführen
		unsigned int current_time = BundleFactory::getDTNTime();

		if ( m_lastyell < current_time )
		{
			// Dazu merken wir uns die Zeit zu der wir zuletzt ein yell() ausgeführt haben.
			m_lastyell = BundleFactory::getDTNTime();

			// ... und führen ein yell() aus.
			yell();
		}

		usleep(1000);
	}

	/*
	 * Sendet eine Nachricht, welche anderen Teilnehmern ermöglicht diesen Teilnehmer
	 * zu erkennen.
	 */
	void EmmaConvergenceLayer::yell()
	{
		/*
		 * Versende ein Broadcast DiscoverPaket mit Daten über Dich selbst.
		 * Damit haben andere DTN Knoten die Möglichkeit Dich zu entdecken.
		 */

		// Erstelle ein DiscoverBundle
		BundleFactory &fac = BundleFactory::getInstance();
		Bundle *bundle = fac.newBundle();
		DiscoverBlock *block = DiscoverBlockFactory::newDiscoverBlock();

		// set address and port
		block->setConnectionAddress(m_bindaddr);
		block->setConnectionPort(m_bindport);

		bundle->appendBlock(block);

		// Setzte den Absender ein
		bundle->setSource( m_eid );

		// Destination
		bundle->setDestination("dtn:discovery");

		// Avoid storing & forwarding
		bundle->setInteger(LIFETIME, 0);

		// Priority expedited
		PrimaryFlags flags = bundle->getPrimaryFlags();
		flags.setPriority(EXPEDITED);
		bundle->setPrimaryFlags(flags);

		// Zusätzliche Daten einsetzen: GPS Position
		block->setLatitude( m_position.first );
		block->setLongitude( m_position.second );

		transmit( *bundle );

		delete bundle;
	}

	void EmmaConvergenceLayer::yell(Node node)
	{
		/*
		 * Versende ein Broadcast DiscoverPaket mit Daten über Dich selbst.
		 * Damit haben andere DTN Knoten die Möglichkeit Dich zu entdecken.
		 */
		// Erstelle ein DiscoverBundle
		BundleFactory &fac = BundleFactory::getInstance();
		Bundle *bundle = fac.newBundle();
		DiscoverBlock *block = DiscoverBlockFactory::newDiscoverBlock();

		// set address and port
		block->setConnectionAddress(m_bindaddr);
		block->setConnectionPort(m_bindport);

		bundle->appendBlock(block);

		// Setzte den Absender ein
		bundle->setSource( m_eid );

		// Destination
		bundle->setDestination( node.getURI() );

		// Avoid storing & forwarding
		bundle->setInteger(LIFETIME, 0);

		// Priority expedited
		PrimaryFlags flags = bundle->getPrimaryFlags();
		flags.setPriority(EXPEDITED);
		bundle->setPrimaryFlags(flags);

		// Zusätzliche Daten einsetzen: GPS Position
		block->setLatitude( m_position.first );
		block->setLongitude( m_position.second );

		transmit( *bundle, node );

		delete bundle;
	}

	void EmmaConvergenceLayer::raiseEvent(const Event *evt)
	{
		const PositionEvent *event = dynamic_cast<const PositionEvent*>(evt);

		if (event != NULL)
		{
			m_position = event->getPosition();
		}
	}
}
