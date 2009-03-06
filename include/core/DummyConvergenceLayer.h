#ifndef DUMMYCONVERGENCELAYER_H_
#define DUMMYCONVERGENCELAYER_H_

#include "ConvergenceLayer.h"

namespace dtn
{
	namespace core
	{
		/**
		 * Diese Klasse implementiert einen ConvergenceLayer als Platzhalter
		 */
		class DummyConvergenceLayer : public ConvergenceLayer
		{
		public:
			DummyConvergenceLayer(string name);
			~DummyConvergenceLayer();

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b)
			 */
			TransmitReport transmit(const Bundle &b);

			/**
			 * @sa protocol::ConvergenceLayer::transmit(Bundle *b, Node &node)
			 */
			TransmitReport transmit(const Bundle &b, const Node &node);

			string getName();

		private:
			string m_name;
		};
	}
}

#endif /*DUMMYCONVERGENCELAYER_H_*/
