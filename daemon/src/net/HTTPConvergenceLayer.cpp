/*
 * HTTPConvergenceLayer.cpp
 *
 *  Created on: 29.07.2010
 *      Author: morgenro
 */

#include "net/HTTPConvergenceLayer.h"
#include <ibrcommon/thread/MutexLock.h>
#include "core/BundleCore.h"
#include <ibrcommon/data/BLOB.h>
#include <ibrdtn/data/Serializer.h>
#include "core/BundleEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "net/BundleReceivedEvent.h"
#include <ibrcommon/Logger.h>

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

namespace dtn
{
	namespace net
	{
		static size_t HTTPConvergenceLayer_callback_read(void *ptr, size_t size, size_t nmemb, void *s)
		{
			size_t retcode = 0;
			std::istream *stream = static_cast<std::istream*>(s);
			char *buffer = static_cast<char*>(ptr);

			stream->read(buffer, (size * nmemb));
			retcode = stream->gcount();

			return retcode;
		}

		static size_t HTTPConvergenceLayer_callback_write(void *ptr, size_t size, size_t nmemb, void *s)
		{
			std::ostream *stream = static_cast<std::ostream*>(s);
			char *buffer = static_cast<char*>(ptr);

			stream->write(buffer, (size * nmemb));

			return (size * nmemb);
		}

		HTTPConvergenceLayer::HTTPConvergenceLayer(const std::string &server)
		 : _server(server)
		{
			curl_global_init(CURL_GLOBAL_ALL);
		}

		HTTPConvergenceLayer::~HTTPConvergenceLayer()
		{
			curl_global_cleanup();
		}

		void HTTPConvergenceLayer::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			try {
				// read the bundle out of the storage
				const dtn::data::Bundle bundle = storage.get(job._bundle);

				ibrcommon::MutexLock l(_write_lock);

				ibrcommon::BLOB::Reference ref = ibrcommon::TmpFileBLOB::create();
				{
					ibrcommon::BLOB::iostream io = ref.iostream();
					dtn::data::DefaultSerializer(*io) << bundle;
				}

				ibrcommon::BLOB::iostream io = ref.iostream();
				size_t length = io.size();
				CURLcode res;

				CURL *curl = curl_easy_init();
				if(curl)
				{
					/* we want to use our own read function */
					curl_easy_setopt(curl, CURLOPT_READFUNCTION, HTTPConvergenceLayer_callback_read);

					/* enable uploading */
					curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

					/* HTTP PUT please */
					curl_easy_setopt(curl, CURLOPT_PUT, 1L);

					/* specify target URL, and note that this URL should include a file
					   name, not only a directory */
					curl_easy_setopt(curl, CURLOPT_URL, _server.c_str());

					/* now specify which file to upload */
					curl_easy_setopt(curl, CURLOPT_READDATA, &(*io));

					/* provide the size of the upload, we specicially typecast the value
					   to curl_off_t since we must be sure to use the correct data size */
					curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
									 (curl_off_t)length);

					/* Now run off and do what you've been told! */
					res = curl_easy_perform(curl);

					/* always cleanup */
					curl_easy_cleanup(curl);

					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
				}
			} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
				// send transfer aborted event
				dtn::net::TransferAbortedEvent::raise(EID(node.getURI()), job._bundle, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
			}
		}

		dtn::core::Node::Protocol HTTPConvergenceLayer::getDiscoveryProtocol() const
		{
			return dtn::core::Node::CONN_HTTP;
		}

		void HTTPConvergenceLayer::componentUp()
		{
		}

		void HTTPConvergenceLayer::componentRun()
		{
			std::string url = _server + "?eid=" + dtn::core::BundleCore::local.getString();
			CURLcode res;

			while (true)
			{
				ibrcommon::BLOB::Reference ref = ibrcommon::TmpFileBLOB::create();
				ibrcommon::BLOB::iostream io = ref.iostream();

				CURL *curl = curl_easy_init();

				if(curl)
				{
					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

					/* no progress meter please */
					curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

					/* send all data to this function  */
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTPConvergenceLayer_callback_write);

					/* now specify which file to upload */
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, &(*io));

					{
						ibrcommon::MutexLock reflock(ref);
						res = curl_easy_perform(curl);
					}

					/* always cleanup */
					curl_easy_cleanup(curl);

					try {
						dtn::data::Bundle bundle;
						dtn::data::DefaultDeserializer(*io) >> bundle;

						// raise default bundle received event
						dtn::net::BundleReceivedEvent::raise(dtn::data::EID(), bundle);
					} catch (ibrcommon::Exception ex) {
						IBRCOMMON_LOGGER_DEBUG(10) << "http error: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
				}
				yield();
			}
		}

		void HTTPConvergenceLayer::componentDown()
		{
		}

		bool HTTPConvergenceLayer::__cancellation()
		{
			// since this is an receiving thread we have to cancel the hard way
			return false;
		}

		const std::string HTTPConvergenceLayer::getName() const
		{
			return "HTTPConvergenceLayer";
		}
	}
}
