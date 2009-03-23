#ifndef MULTIPLEXCONVERGENCELAYER_H_
#define MULTIPLEXCONVERGENCELAYER_H_

#include "core/ConvergenceLayer.h"
#include "core/BundleReceiver.h"
#include "utils/Service.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"

#include <queue>
#include <list>

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		/**
		 * This class provider a multiplexing component for several ConvergenceLayer
		 * instances.
		 */
		class MultiplexConvergenceLayer : public ConvergenceLayer, public BundleReceiver
		{
		public:
			/**
			 * constructor
			 */
			MultiplexConvergenceLayer();

			/**
			 * destructor
			 */
			virtual ~MultiplexConvergenceLayer();

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b)
			 */
			virtual TransmitReport transmit(const Bundle &b);

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
			 */
			virtual TransmitReport transmit(const Bundle &b, const Node &node);

			/**
			 * Add a ConvergenceLayer instance.
			 */
			void add(ConvergenceLayer *cl);

			void received(const ConvergenceLayer &cl, const dtn::data::Bundle &b);

			void initialize();
			void terminate();

		private:
			list<ConvergenceLayer*> m_clayers;
			dtn::utils::Mutex m_receivelock;
		};
	}
}

#endif /*MULTIPLEXCONVERGENCELAYER_H_*/
