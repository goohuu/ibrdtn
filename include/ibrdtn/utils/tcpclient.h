/*
 * tcpclient.h
 *
 *  Created on: 29.07.2009
 *      Author: morgenro
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/streams/tcpstream.h"

namespace dtn
{
	namespace utils
	{
		class tcpclient : public dtn::streams::tcpstream
		{
		public:
			class SocketException : public dtn::exceptions::Exception
			{
			public:
				SocketException(string error) : Exception(error)
				{};
			};

			tcpclient(string address, int port);
			virtual ~tcpclient();

			void close();
		};
	}
}

#endif /* TCPCLIENT_H_ */
