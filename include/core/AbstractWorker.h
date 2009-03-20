#ifndef ABSTRACTWORKER_H_
#define ABSTRACTWORKER_H_

#include "core/BundleCore.h"
#include "core/ConvergenceLayer.h"
#include "data/Bundle.h"
#include "core/EventSwitch.h"
#include "core/RouteEvent.h"
using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		class AbstractWorker
		{
			public:
				AbstractWorker(const string uri) : m_uri(uri)
				{
					BundleCore &core = BundleCore::getInstance();

					// Registriere mich als Knoten der Bundle empfangen kann
					core.registerSubNode( getWorkerURI(), this );
				};

				virtual ~AbstractWorker() {
					BundleCore &core = BundleCore::getInstance();
					core.unregisterSubNode( getWorkerURI() );
				};

				virtual const string getWorkerURI() const
				{
					return m_uri;
				}

				virtual TransmitReport callbackBundleReceived(const Bundle &b) = 0;

			protected:
				void transmit(const Bundle &bundle)
				{
					EventSwitch::raiseEvent(new RouteEvent(bundle, ROUTE_PROCESS_BUNDLE));
				}

			private:
				string m_uri;
		};
	}
}

#endif /*ABSTRACTWORKER_H_*/
