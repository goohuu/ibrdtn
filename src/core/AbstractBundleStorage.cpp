/*
 * AbstractBundleStorage.cpp
 *
 *  Created on: 24.03.2009
 *      Author: morgenro
 */

#include "core/AbstractBundleStorage.h"

#include "data/BundleFactory.h"
#include "utils/Utils.h"
#include "core/NodeEvent.h"
#include "core/StorageEvent.h"
#include "core/EventSwitch.h"
#include "core/BundleEvent.h"
#include "core/RouteEvent.h"
#include "core/CustodyEvent.h"
#include "core/TimeEvent.h"


namespace dtn
{
	namespace core
	{
		AbstractBundleStorage::AbstractBundleStorage()
		{
			// register me for events
			EventSwitch::registerEventReceiver( NodeEvent::className, this );
			EventSwitch::registerEventReceiver( StorageEvent::className, this );
			EventSwitch::registerEventReceiver( TimeEvent::className, this );
		}

		AbstractBundleStorage::~AbstractBundleStorage()
		{
			EventSwitch::unregisterEventReceiver( NodeEvent::className, this );
			EventSwitch::unregisterEventReceiver( StorageEvent::className, this );
			EventSwitch::unregisterEventReceiver( TimeEvent::className, this );
			clear();
		}

		void AbstractBundleStorage::wait()
		{
			m_breakwait.wait();
		}

		void AbstractBundleStorage::stopWait()
		{
			m_breakwait.signal();
		}

		void AbstractBundleStorage::raiseEvent(const Event *evt)
		{
			const NodeEvent *node = dynamic_cast<const NodeEvent*>(evt);
			const StorageEvent *storage = dynamic_cast<const StorageEvent*>(evt);
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

			if (time != NULL)
			{
				if (time->getAction() == TIME_SECOND_TICK)
				{
					// the time has changed
					m_breakwait.signal();
				}
			}
			else if (storage != NULL)
			{
				switch (storage->getAction())
				{
					case STORE_BUNDLE:
					{
						Bundle *ret = storeFragment(storage->getBundle());
						m_breakwait.signal();

						if (ret != NULL)
						{
							// route the joint bundle
							EventSwitch::raiseEvent( new RouteEvent(*ret, ROUTE_PROCESS_BUNDLE) );
							delete ret;
						}

						break;
					}

					case STORE_SCHEDULE:
						const BundleSchedule &sched = storage->getSchedule();

						try {
							// local eid
							string localeid = BundleCore::getInstance().getLocalURI();
							const Bundle &b = sched.getBundle();

							if ( ( b.getPrimaryFlags().isCustodyRequested() ) && (b.getCustodian() != localeid) )
							{
								// here i need a copy
								Bundle b_copy = b;

								// set me as custody
								b_copy.setCustodian(localeid);

								// Make a new schedule
								BundleSchedule sched_new(b_copy, sched.getTime(), sched.getEID());

								// store the schedule
								store(sched_new);
							}
							else
							{
								// store the schedule
								store(sched);
							}

							// accept custody
							EventSwitch::raiseEvent( new CustodyEvent( sched.getBundle(), CUSTODY_ACCEPT ) );

						} catch (NoSpaceLeftException ex) {
							// reject custody
							EventSwitch::raiseEvent( new CustodyEvent( sched.getBundle(), CUSTODY_REJECT ) );
						}

						m_breakwait.signal();
						break;
				}
			}
			else if (node != NULL)
			{
				const Node &n = node->getNode();

				switch (node->getAction())
				{
					case NODE_AVAILABLE:
						eventNodeAvailable(n);
						break;

					case NODE_UNAVAILABLE:
						eventNodeUnavailable(n);
						break;
				}
			}
		}
	}
}
