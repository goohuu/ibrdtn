#ifndef BUNDLEROUTER_H_
#define BUNDLEROUTER_H_

#include "Node.h"
#include <list>
#include "data/Bundle.h"
#include "utils/Service.h"
#include "core/BundleSchedule.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include "core/EventReceiver.h"
#include "core/Event.h"

using namespace std;
using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		/**
		 * Ein BundleRouter entscheidet über die weitergabe der Pakete. Er kann gefragt
		 * werden zu welchen Knoten ein Bundle als nächstes übertragen werden und
		 * wann das statt finden soll.
		 */
		class BundleRouter : public Service, public EventReceiver
		{
		public:
			/* Konstruktor
			 * @param eid Spezifiziert die eigene EID
			 */
			BundleRouter(string eid);

			/**
			 * Destruktor
			 */
			virtual ~BundleRouter();

			/**
			 * Gibt alle direkten nodegenceLayerbestimmtNachbarn zurück
			 * @return Ein vector der Zeiger auf alle Nachbarn beinhaltet die entdeckt wurden.
			 */
			virtual list<Node> getNeighbours();

			/**
			 * Emittelt ob ein bestimmter Knoten ein direkter Nachbar ist oder nicht
			 * @param node Der zu bestimmende Knoten
			 * @return True, wenn der Knoten ein direkter Nachbar ist
			 */
			virtual bool isNeighbour(Node &node);
			bool isNeighbour(string eid);

			Node getNeighbour(string eid);

			/**
			 * @sa Service::tick()
			 */
			virtual void tick();

			/**
			 * Gibt einen Schedule für ein bestimmtes Bundle zurück
			 */
			virtual BundleSchedule getSchedule(const Bundle &b);

			bool isLocal(const Bundle &b) const;

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const Event *evt);

		private:
			/**
			 * Gibt an, dass ein bestimmter Knoten nun in Kommunikationsreichweite ist.
			 * @param node Ein Zeiger auf ein Node-Objekt der die Daten des in Reichweite gekommenen Knoten enthält.
			 */
			virtual void discovered(const Node &node);

			list<Node> m_neighbours;
			unsigned int m_lastcheck;
			string m_eid;
			dtn::utils::Mutex m_lock;
		};
	}
}

#endif /*BUNDLEROUTER_H_*/
