/*
 * TransferAbortedEvent.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "net/TransferAbortedEvent.h"

namespace dtn
{
	namespace net
	{
		TransferAbortedEvent::TransferAbortedEvent(const dtn::data::EID &peer, const dtn::data::Bundle &bundle, const AbortReason r)
		 : _peer(peer), _bundle(bundle), reason(r)
		{
		}

		TransferAbortedEvent::TransferAbortedEvent(const dtn::data::EID &peer, const dtn::data::BundleID &id, const AbortReason r)
		 : _peer(peer), _bundle(id), reason(r)
		{
		}

		TransferAbortedEvent::~TransferAbortedEvent()
		{

		}

		void TransferAbortedEvent::raise(const dtn::data::EID &peer, const dtn::data::Bundle &bundle, const AbortReason r)
		{
			// raise the new event
			dtn::core::Event::raiseEvent( new TransferAbortedEvent(peer, bundle, r) );
		}

		void TransferAbortedEvent::raise(const dtn::data::EID &peer, const dtn::data::BundleID &id, const AbortReason r)
		{
			// raise the new event
			dtn::core::Event::raiseEvent( new TransferAbortedEvent(peer, id, r) );
		}

		const std::string TransferAbortedEvent::getName() const
		{
			return TransferAbortedEvent::className;
		}

		dtn::data::EID TransferAbortedEvent::getPeer() const
		{
			return _peer;
		}

		dtn::data::BundleID TransferAbortedEvent::getBundleID() const
		{
			return _bundle;
		}

		string TransferAbortedEvent::toString() const
		{
			return className + ": transfer of bundle " + _bundle.toString() + " to " + _peer.getString() + " aborted.";
		}

		const std::string TransferAbortedEvent::className = "TransferAbortedEvent";
	}
}
