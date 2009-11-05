#ifndef CONVERGENCELAYER_H_
#define CONVERGENCELAYER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Bundle.h"
#include "net/BundleConnection.h"
#include "core/Node.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
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
			 * destructor
			 */
			virtual ~ConvergenceLayer() {};

			virtual dtn::net::BundleConnection* getConnection(const dtn::core::Node &node) = 0;

			void setBundleReceiver(BundleReceiver *receiver);
			void eventBundleReceived(const Bundle &bundle);

		private:
			BundleReceiver *m_receiver;
		};
	}
}

#endif /*CONVERGENCELAYER_H_*/
