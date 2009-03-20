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

#include "core/EventReceiver.h"

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
	class EmmaConvergenceLayer : public dtn::core::ConvergenceLayer, public BundleReceiver, public EventReceiver
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
		virtual TransmitReport transmit(const Bundle &b);

		/**
		 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
		 */
		virtual TransmitReport transmit(const Bundle &b, const Node &node);

		void received(const ConvergenceLayer &cl, const Bundle &b);

		/**
		 * method to receive PositionEvent from EventSwitch
		 */
		void raiseEvent(const Event *evt);

//	protected:
//		/**
//		 * @sa Service::tick()
//		 */
//		virtual void tick();

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

		UDPConvergenceLayer *m_direct_cl;
		UDPConvergenceLayer *m_broadcast_cl;

		Mutex m_receivelock;

		pair<double,double> m_position;

		DiscoverBlockFactory *m_dfactory;
	};
}

#endif /*EMMACONVERGENCELAYER_H_*/
