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

		dtn::data::BundleID TransferCompletedEvent::getBundleID() const
		{
			return _bundle;
		}

#ifdef DO_DEBUG_OUTPUT
		string TransferCompletedEvent::toString() const
		{
			return className + ": transfer of bundle " + _bundle.toString() + " to " + _peer.getString() + " completed";
		}
#endif

		const string TransferCompletedEvent::className = "TransferCompletedEvent";
	}
}
