#ifndef ABSTRACTWORKER_H_
#define ABSTRACTWORKER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/utils/Mutex.h"
#include "net/ConvergenceLayer.h"

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		class AbstractWorker : public dtn::utils::Mutex
		{
			public:
				AbstractWorker();

				AbstractWorker(const string uri);

				virtual ~AbstractWorker();

				virtual const EID getWorkerURI() const;

				virtual dtn::net::TransmitReport callbackBundleReceived(const Bundle &b) = 0;

			protected:
				void transmit(const Bundle &bundle);

			private:
				EID m_uri;
		};
	}
}

#endif /*ABSTRACTWORKER_H_*/
