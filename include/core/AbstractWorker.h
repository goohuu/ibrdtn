#ifndef ABSTRACTWORKER_H_
#define ABSTRACTWORKER_H_

#include "core/BundleCore.h"
#include "core/ConvergenceLayer.h"
#include "data/Bundle.h"

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		class AbstractWorker
		{
			public:
				AbstractWorker(BundleCore *core, const string uri) : m_core(core), m_uri(uri)
				{
					// Registriere mich als Knoten der Bundle empfangen kann
					m_core->registerSubNode( getWorkerURI(), this );
				};

				virtual ~AbstractWorker() {
					m_core->unregisterSubNode( getWorkerURI() );
				};

				BundleCore *getCore()
				{
					return m_core;
				};

				virtual const string getWorkerURI() const
				{
					return m_uri;
				}

				virtual TransmitReport callbackBundleReceived(Bundle *b)
				{
					if (b == NULL)
					{
						return UNKNOWN;
					}

					return UNKNOWN;
				};

			private:
				BundleCore *m_core;
				string m_uri;
		};
	}
}

#endif /*ABSTRACTWORKER_H_*/
