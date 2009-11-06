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
			class AbstractWorkerAsync : public dtn::utils::JoinableThread
			{
			public:
				AbstractWorkerAsync(AbstractWorker &worker);
				~AbstractWorkerAsync();
				void shutdown();

			protected:
				void run();

			private:
				AbstractWorker &_worker;
				bool _running;
			};

			public:
				AbstractWorker();

				virtual ~AbstractWorker();

				virtual const EID getWorkerURI() const;

				virtual dtn::net::TransmitReport callbackBundleReceived(const Bundle &b) = 0;

			protected:
				void initialize(const string uri, bool async = false);
				void transmit(const Bundle &bundle);
				dtn::data::Bundle receive();

				EID _eid;

				void shutdown();

			private:
				AbstractWorkerAsync _thread;
		};
	}
}

#endif /*ABSTRACTWORKER_H_*/
