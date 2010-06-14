/*
 * TransferCompletedEvent.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "net/TransferCompletedEvent.h"
#include "core/BundleCore.h"

namespace dtn
{
	namespace net
	{
		TransferCompletedEvent::TransferCompletedEvent(const dtn::data::EID peer, const dtn::data::Bundle &bundle)
		 : _peer(peer), _bundle(bundle)
		{

		}

		TransferCompletedEvent::~TransferCompletedEvent()
		{

		}

		void TransferCompletedEvent::raise(const dtn::data::EID peer, const dtn::data::Bundle &bundle)
		{
			// raise the new event
			dtn::core::Event::raiseEvent( new TransferCompletedEvent(peer, bundle) );
		}

		const string TransferCompletedEvent::getName() const
		{
			return TransferCompletedEvent::className;
		}

		dtn::data::EID TransferCompletedEvent::getPeer() const
		{
			return _peer;
		}

		dtn::routing::MetaBundle TransferCompletedEvent::getBundle() const
		{
			return _bundle;
		}

		string TransferCompletedEvent::toString() const
		{
			return className + ": transfer of bundle " + _bundle.toString() + " to " + _peer.getString() + " completed";
		}

		const string TransferCompletedEvent::className = "TransferCompletedEvent";
	}
}
