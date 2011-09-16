#include "net/LOWPANConnection.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "core/BundleCore.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>

#include <ibrdtn/data/ScopeControlHopLimitBlock.h>
#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/data/Serializer.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <list>

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		LOWPANConnection::LOWPANConnection(unsigned short address, LOWPANConvergenceLayer &cl)
			: address(address), _stream(cl, address), _sender(_stream)
		{
		}

		LOWPANConnection::~LOWPANConnection()
		{

		}

		lowpanstream& LOWPANConnection::getStream()
		{
			return _stream;
		}

		void LOWPANConnection::run()
		{
			while(_stream.good())
			{
				try {
					dtn::data::DefaultDeserializer deserializer(_stream);
					dtn::data::Bundle bundle;
					deserializer >> bundle;



					// determine sender
					EID sender;

					// increment value in the scope control hop limit block
					try {
						dtn::data::ScopeControlHopLimitBlock &schl = bundle.getBlock<dtn::data::ScopeControlHopLimitBlock>();
						schl.increment();
					} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) { };

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(sender, bundle);

				} catch (const dtn::InvalidDataException &ex) {
					IBRCOMMON_LOGGER(warning) << "Received a invalid bundle: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				} catch (const ibrcommon::IOException&) {

				}
			}
		}

		// class LOWPANConnectionSender
		LOWPANConnectionSender::LOWPANConnectionSender(lowpanstream &stream)
		: stream(stream)
		{
		}

		LOWPANConnectionSender::~LOWPANConnectionSender()
		{
		}

		void LOWPANConnectionSender::queue(const ConvergenceLayer::Job &job)
		{
			_queue.push(job);
		}

		void LOWPANConnectionSender::run()
		{
			while(stream.good())
			{
				ConvergenceLayer::Job job = _queue.getnpop();
				dtn::data::DefaultSerializer serializer(stream);

				dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

				// read the bundle out of the storage
				const dtn::data::Bundle bundle = storage.get(job._bundle);

				// Put bundle into stringstream
				serializer << bundle; stream.flush();
				// raise bundle event
				dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
				dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
			}
			// FIXME: Exit strategy when sending on socket failed. Like destroying the connection object
			// Also check what needs to be done when the node is not reachable (transfer requeue...)
		}
	}
}
