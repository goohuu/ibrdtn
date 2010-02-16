/*
 * TransferAbortedEvent.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "net/TransferAbortedEvent.h"
#include "core/BundleCore.h"

namespace dtn
{
	namespace net
	{
		TransferAbortedEvent::TransferAbortedEvent(const dtn::data::EID peer, const dtn::data::Bundle &bundle)
		 : _peer(peer), _bundle(bundle)
		{

		}

		TransferAbortedEvent::~TransferAbortedEvent()
		{

		}

		void TransferAbortedEvent::raise(const dtn::data::EID peer, const dtn::data::Bundle &bundle)
		{
			// raise the new event
			dtn::core::Event::raiseEvent( new TransferAbortedEvent(peer, bundle) );
		}

		const string TransferAbortedEvent::getName() const
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

#ifdef DO_DEBUG_OUTPUT
		string TransferAbortedEvent::toString() const
		{
			return className + ": transfer of bundle " + _bundle.toString() + " to " + _peer.getString() + " aborted.";
		}
#endif

		const string TransferAbortedEvent::className = "TransferAbortedEvent";
	}
}
