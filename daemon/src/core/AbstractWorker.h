#ifndef ABSTRACTWORKER_H_
#define ABSTRACTWORKER_H_

#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/BundleID.h>
#include <ibrdtn/data/EID.h>
#include "core/EventReceiver.h"
#include <ibrcommon/thread/Mutex.h>
#include <ibrcommon/thread/Conditional.h>
#include <ibrcommon/thread/Thread.h>
#include "net/ConvergenceLayer.h"

#include <ibrcommon/thread/Queue.h>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		class AbstractWorker : public ibrcommon::Mutex
		{
			class AbstractWorkerAsync : public ibrcommon::JoinableThread, public dtn::core::EventReceiver
			{
			public:
				AbstractWorkerAsync(AbstractWorker &worker);
				virtual ~AbstractWorkerAsync();
				void shutdown();

				virtual void raiseEvent(const dtn::core::Event *evt);

			protected:
				void run();
				bool __cancellation();

			private:
				AbstractWorker &_worker;
				bool _running;

				ibrcommon::Queue<dtn::data::BundleID> _receive_bundles;
			};

			public:
				AbstractWorker();

				virtual ~AbstractWorker();

				virtual const EID getWorkerURI() const;

				virtual void callbackBundleReceived(const Bundle &b) = 0;

			protected:
				void initialize(const string uri, bool async = false);
				void transmit(const Bundle &bundle);
				//dtn::data::Bundle receive();

				EID _eid;

				void shutdown();

			private:
				AbstractWorkerAsync _thread;
		};
	}
}

#endif /*ABSTRACTWORKER_H_*/
