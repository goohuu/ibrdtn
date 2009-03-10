#ifndef CONVERGENCELAYER_H_
#define CONVERGENCELAYER_H_

#include "data/Bundle.h"
#include "Node.h"

using namespace dtn::data;

namespace dtn
{
namespace core
{
	class BundleReceiver;

	/**
	 * Übermittlungsbericht
	 * Rückgabewert von transmit() der angibt ob ein Bundle zugestellt werden konnte oder ggf. warum nicht.
	 * @sa transmit()
	 */
	enum TransmitReport
	{
		UNKNOWN = -1,
		TRANSMIT_SUCCESSFUL = 0,
		NO_ROUTE_FOUND = 1,
		CONVERGENCE_LAYER_BUSY = 2,
		BUNDLE_ACCEPTED = 3
	};

	/**
	 * Ist für die Zustellung von Bundles verantwortlich.
	 */
	class ConvergenceLayer
	{
		public:
		/**
		 * Konstruktor
		 */
		ConvergenceLayer() : m_receiver(NULL)
		{};

		/**
		 * Destruktor
		 */
		virtual ~ConvergenceLayer() {};

		/**
		 * Nimmt ein Bundle zur Übermittlung entgegen und liefert einen TransmitReport zurück
		 * @param b Das zu übermittelnde Bundle
		 * @return Ein TransmitReport der wiedergibt, ob ein Bundle zugestellt werden konnte oder nicht.
		 */
		virtual TransmitReport transmit(const Bundle &b) = 0;

		/**
		 * Nimmt ein Bundle zur Übermittlung an einen bestimmten Knoten entgegen und
		 * liefert einen TransmitReport zurück
		 * @param b Das zu übermittelnde Bundle
		 * @param node Der Knoten an den diese Bundle direkt ausgeliefert werden soll
		 * @return Ein TransmitReport der wiedergibt, ob ein Bundle zugestellt werden konnte oder nicht.
		 */
		virtual TransmitReport transmit(const Bundle &b, const Node &node) = 0;

		void setBundleReceiver(BundleReceiver *receiver);
		void eventBundleReceived(Bundle &bundle);

		private:
		BundleReceiver *m_receiver;
	};
}
}

#endif /*CONVERGENCELAYER_H_*/
