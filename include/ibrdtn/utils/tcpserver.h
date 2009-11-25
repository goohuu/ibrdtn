/*
 * tcpserver.h
 *
 *  Created on: 29.07.2009
 *      Author: morgenro
 */

#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/tcpstream.h"
#include <netinet/in.h>
#include "ibrdtn/utils/NetInterface.h"

namespace dtn
{
	namespace utils
	{
		class tcpserver
		{
		public:
			class SocketException : public dtn::exceptions::Exception
			{
			public:
				SocketException(string error) : Exception(error)
				{};
			};

			/**
			 * @param address the address to listen to
			 * @param port the port to listen to
			 */
			tcpserver(dtn::net::NetInterface net);

			/**
			 * Destructor
			 */
			virtual ~tcpserver();

			/**
			 * Wait for a new connection a specific time.
			 * @param timeout The time to wait for a new connection in milliseconds
			 * @return True, if a new connection is available.
			 */
			bool waitPending(int timeout = -1);

			/**
			 * Accept a new connection.
			 * @return A socket for the connection.
			 */
			int accept();

		protected:
			void close();

		private:
			int _socket;
			bool _closed;

		};
	}
}

#endif /* TCPSERVER_H_ */
