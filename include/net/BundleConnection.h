/*
 * BundleConnection.h
 *
 *  Created on: 18.09.2009
 *      Author: morgenro
 */

#ifndef BUNDLECONNECTION_H_
#define BUNDLECONNECTION_H_

#include "ibrdtn/config.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrcommon/thread/Thread.h"
#include "ibrdtn/data/Exceptions.h"
#include <iostream>

namespace dtn
{
	namespace net
	{
		class BundleConnection : public ibrcommon::Mutex
		{
			class BundleReceiver : public ibrcommon::JoinableThread
			{
			public:
				BundleReceiver(BundleConnection &connection);
				virtual ~BundleReceiver();
				void run();
				virtual void shutdown();

			private:
				bool _running;
				BundleConnection &_connection;
			};

		public:
                    class ConnectionInterruptedException : public dtn::exceptions::IOException
                    {
                    public:
                        ConnectionInterruptedException(size_t last_ack = 0) : dtn::exceptions::IOException("The connection has been interrupted."), _last_ack(last_ack)
                        {
                        }
                        
                        size_t getLastAck()
                        {
                            return _last_ack;
                        }
                    private:
                        size_t _last_ack;
                    };

			BundleConnection();
			virtual ~BundleConnection();

			virtual void read(dtn::data::Bundle &bundle) = 0;
			virtual void write(const dtn::data::Bundle &bundle) = 0;

			virtual void initialize();
			virtual void shutdown();

			void waitFor();

		private:
			BundleReceiver _receiver;
		};
	}
}

#endif /* BUNDLECONNECTION_H_ */
