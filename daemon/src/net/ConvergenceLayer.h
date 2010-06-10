#ifndef CONVERGENCELAYER_H_
#define CONVERGENCELAYER_H_

#include "ibrdtn/data/Bundle.h"
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
			class Job
			{
			public:
				Job(const dtn::data::EID &eid, const dtn::data::Bundle &b);
				~Job();

				dtn::data::Bundle _bundle;
				dtn::data::EID _destination;
			};

			/**
			 * Konstruktor
			 */
			ConvergenceLayer() : m_receiver(NULL)
			{};

			/**
			 * destructor
			 */
			virtual ~ConvergenceLayer() {};

			void setBundleReceiver(BundleReceiver *receiver);
			void eventBundleReceived(const Bundle &bundle);

			virtual const dtn::core::NodeProtocol getDiscoveryProtocol() const = 0;

			virtual void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job) = 0;

		private:
			BundleReceiver *m_receiver;
		};
	}
}

#endif /*CONVERGENCELAYER_H_*/
