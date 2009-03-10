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
		 * Diese Klasse implementiert einen ConvergenceLayer für UDP/IP
		 */
		class MultiplexConvergenceLayer : public ConvergenceLayer, public BundleReceiver
		{
		public:
			/**
			 * Konstruktor
			 */
			MultiplexConvergenceLayer();

			/**
			 * Desktruktor
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
			 * Fügt dem Multiplexer einen ConvergenceLayer hinzu
			 */
			void add(ConvergenceLayer *cl);

			void received(const ConvergenceLayer &cl, dtn::data::Bundle &b);

			void initialize();
			void terminate();

		private:
			list<ConvergenceLayer*> m_clayers;
			dtn::utils::Mutex m_receivelock;
		};
	}
}

#endif /*MULTIPLEXCONVERGENCELAYER_H_*/
