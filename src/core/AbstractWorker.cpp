/*
 * AbstractWorker.cpp
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#include "core/AbstractWorker.h"
#include "core/BundleCore.h"
#include "core/RouteEvent.h"
#include "core/EventSwitch.h"


namespace dtn
{
	namespace core
	{
		AbstractWorker::AbstractWorker() : m_uri("dtn:none")
		{
		};

		AbstractWorker::AbstractWorker(const string uri)
		{
			BundleCore &core = BundleCore::getInstance();
			m_uri = BundleCore::local + uri;

			// Registriere mich als Knoten der Bundle empfangen kann
			core.registerSubNode( getWorkerURI(), this );
		};

		AbstractWorker::~AbstractWorker()
		{
			dtn::utils::MutexLock l(*this);

			if (m_uri != EID())
			{
				BundleCore &core = BundleCore::getInstance();
				core.unregisterSubNode( getWorkerURI() );
			}
		};

		const EID AbstractWorker::getWorkerURI() const
		{
			return m_uri;
		}

		void AbstractWorker::transmit(const Bundle &bundle)
		{
			EventSwitch::raiseEvent(new RouteEvent(bundle, ROUTE_PROCESS_BUNDLE));
		}
	}
}
