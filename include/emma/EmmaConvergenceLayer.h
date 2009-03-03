#ifndef EMMACONVERGENCELAYER_H_
#define EMMACONVERGENCELAYER_H_

#include "core/ConvergenceLayer.h"
#include "core/UDPConvergenceLayer.h"
#include "core/BundleReceiver.h"
#include "data/BundleFactory.h"
#include "emma/DiscoverBlockFactory.h"
#include "emma/GPSProvider.h"
#include "core/BundleRouter.h"
#include "utils/Service.h"

#include "utils/MutexLock.h"
#include "utils/Mutex.h"

#include <queue>

using namespace dtn::data;
using namespace dtn::core;
using namespace dtn::utils;

namespace emma
{
	/**
	 * Diese Klasse implementiert einen ConvergenceLayer für UDP/IP über IEEE 802.11
	 * welche für die Kommunikation zwischen den Fahrzeugen im EMMA Projekt benötigt wird.
	 */
	class EmmaConvergenceLayer : public Service, public dtn::core::ConvergenceLayer, public BundleReceiver
	{
	public:
		/**
		 * Constructor
		 * @param[in] eid The own eid for broadcasting discovery bundles.
		 * @param[in] bind_addr The own ip address to bind on.
		 * @param[in] port The udp port to use.
		 * @param[in] broadcast The broadcast ip address.
		 * @param[in] mtu The maximum bundle size.
		 */
		EmmaConvergenceLayer(string eid, string bind_addr, unsigned short port, string broadcast, unsigned int mtu = 1280);

		/**
		 * Desktruktor
		 */
		virtual ~EmmaConvergenceLayer();

		/**
		 * @sa protocol::ConvergenceLayer::transmit(Bundle *b)
		 */
		virtual TransmitReport transmit(Bundle *b);

		/**
		 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
		 */
		virtual TransmitReport transmit(Bundle *b, const Node &node);

		/**
		 * Gibt den zugewiesenen Router zurück
		 * @return Ein Router-Objekt, welches für die Verarbeitung von eingehenden Bundles zuständig ist.
		 */
		BundleRouter* getRouter();

		/**
		 * Setzt den Router
		 * @param router Ein Router an den eingehende Bundles weitergegeben werden sollen.
		 */
		void setRouter(BundleRouter *router);

		void setGPSProvider(GPSProvider *gpsconn);

		void received(ConvergenceLayer *cl, Bundle *b);

	protected:
		/**
		 * @sa Service::tick()
		 */
		virtual void tick();

		virtual void initialize();

		virtual void terminate();

	private:
		/**
		 * Sendet eine Nachricht, welche anderen Teilnehmern ermöglicht diesen Teilnehmer
		 * zu erkennen.
		 */
		void yell();
		void yell(Node node);

		static const int MAX_SIZE;

		string m_eid;
		string m_bindaddr;
		unsigned int m_bindport;

		BundleRouter *m_router;

		unsigned int m_lastyell;

		UDPConvergenceLayer *m_direct_cl;
		UDPConvergenceLayer *m_broadcast_cl;

		Mutex m_receivelock;

		GPSProvider *m_gps;

		DiscoverBlockFactory *m_dfactory;
	};
}

#endif /*EMMACONVERGENCELAYER_H_*/
