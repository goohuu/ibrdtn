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
