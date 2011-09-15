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
		 : address(address)
		{
		}

		LOWPANConnection::~LOWPANConnection()
		{

		}

		lowpanstream& LOWPANConnection::getStream()
		{
			return *_stream;
		}

		void LOWPANConnection::run()
		{
		}

		// class LOWPANConnectionSender
		LOWPANConnectionSender::LOWPANConnectionSender(lowpanstream &stream)
		: stream(stream)
		{
		}

		LOWPANConnectionSender::~LOWPANConnectionSender()
		{
		}

		void LOWPANConnectionSender::queue(const BundleID &id)
		{
			dtn::data::DefaultSerializer serializer(stream);

			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			// read the bundle out of the storage
			const dtn::data::Bundle bundle = storage.get(id);

			unsigned int size = serializer.getLength(bundle);

			//const dtn::core::Node::URI &uri = uri_list.front();
			//std::string address;
			//unsigned int pan;
			// read values
			//uri.decode(address, pan);

			// Put bundle into stringstream
			serializer << bundle;
			// Call sync() to make sure the stream knows about the last segment
		}

		void LOWPANConnectionSender::run()
		{
		}
	}
}
