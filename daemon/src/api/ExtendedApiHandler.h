/*
 * ExtendedApiHandler.h
 *
 *  Created on: 10.10.2011
 *      Author: morgenro,roettger
 */

#ifndef EXTENDEDAPIHANDLER_H_
#define EXTENDEDAPIHANDLER_H_

#include "api/Registration.h"
#include "core/Node.h"
#include "api/ClientHandler.h"

#include <ibrdtn/data/Bundle.h>
#include <ibrcommon/thread/Thread.h>
#include <ibrcommon/thread/Queue.h>
#include <ibrcommon/net/tcpstream.h>


namespace dtn
{
	namespace api
	{
		class ExtendedApiHandler : public ProtocolHandler
		{
		public:
			enum STATUS_CODES
			{
				API_STATUS_CONTINUE = 100,
				API_STATUS_OK = 200,
				API_STATUS_CREATED = 201,
				API_STATUS_ACCEPTED = 202,
				API_STATUS_FOUND = 302,
				API_STATUS_BAD_REQUEST = 400,
				API_STATUS_UNAUTHORIZED = 401,
				API_STATUS_FORBIDDEN = 403,
				API_STATUS_NOT_FOUND = 404,
				API_STATUS_NOT_ALLOWED = 405,
				API_STATUS_NOT_ACCEPTABLE = 406,
				API_STATUS_CONFLICT = 409,
				API_STATUS_INTERNAL_ERROR = 500,
				API_STATUS_NOT_IMPLEMENTED = 501,
				API_STATUS_SERVICE_UNAVAILABLE = 503,
				API_STATUS_VERSION_NOT_SUPPORTED = 505,
				API_STATUS_NOTIFY_COMMON = 600,
				API_STATUS_NOTIFY_NEIGHBOR = 601,
				API_STATUS_NOTIFY_BUNDLE = 602,
			};

			ExtendedApiHandler(ClientHandler &client, ibrcommon::tcpstream &stream);
			virtual ~ExtendedApiHandler();

			virtual void run();
			virtual void finally();
			virtual bool __cancellation();

			Registration& getRegistration();

			bool good() const;

//			void eventNodeAvailable(const dtn::core::Node &node);
//			void eventNodeUnavailable(const dtn::core::Node &node);

		private:
			class Sender : public ibrcommon::JoinableThread
			{
			public:
				Sender(ExtendedApiHandler &conn);
				virtual ~Sender();

			protected:
				void run();
				void finally();
				bool __cancellation();

			private:
				ExtendedApiHandler &_handler;
			} _sender;

			static void sayBundleID(ostream &stream, const dtn::data::BundleID &id);
			static dtn::data::BundleID readBundleID(const std::vector<std::string>&, const size_t start);

			void processIncomingBundle(dtn::data::Bundle &b);

			Registration &_registration;
			ibrcommon::Mutex _write_lock;

			dtn::data::Bundle _bundle_reg;
			dtn::data::EID _endpoint;
			ibrcommon::Queue<dtn::data::BundleID> _bundle_queue;
		};
	}
}

#endif /* EXTENDEDAPICONNECTION_H_ */
