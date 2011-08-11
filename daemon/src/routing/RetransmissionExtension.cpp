/*
 * RetransmissionExtension.cpp
 *
 *  Created on: 09.03.2010
 *      Author: morgenro
 */

#include "routing/RetransmissionExtension.h"
#include "core/TimeEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "net/TransferAbortedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "core/BundleExpiredEvent.h"
#include "ibrdtn/utils/Clock.h"
#include "ibrdtn/data/Exceptions.h"
#include "core/BundleCore.h"

namespace dtn
{
	namespace routing
	{
		RetransmissionExtension::RetransmissionExtension()
		{
		}

		RetransmissionExtension::~RetransmissionExtension()
		{
		}

		void RetransmissionExtension::notify(const dtn::core::Event *evt)
		{
			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				if (!_queue.empty())
				{
					const RetransmissionData &data = _queue.front();

					if ( data.getTimestamp() <= time.getTimestamp() )
					{
						// retransmit the bundle
						dtn::core::BundleCore::getInstance().transferTo(data.destination, data);

						// remove the item off the queue
						_queue.pop();
					}
				}
				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);

				// remove the bundleid in our list
				RetransmissionData data(completed.getBundle(), completed.getPeer());
				_set.erase(data);

				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);

				// remove the bundleid in our list
				RetransmissionData data(aborted.getBundleID(), aborted.getPeer());
				_set.erase(data);

				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::routing::RequeueBundleEvent &requeue = dynamic_cast<const dtn::routing::RequeueBundleEvent&>(*evt);

				const RetransmissionData data(requeue._bundle, requeue._peer);
				std::set<RetransmissionData>::const_iterator iter = _set.find(data);

				if (iter != _set.end())
				{
					// increment the retry counter
					RetransmissionData data2 = (*iter);
					data2++;

					// remove the item
					_set.erase(data);

					if (data2.getCount() <= 8)
					{
						// requeue the bundle
						_set.insert(data2);
						_queue.push(data2);
					}
					else
					{
						dtn::net::TransferAbortedEvent::raise(requeue._peer, requeue._bundle, dtn::net::TransferAbortedEvent::REASON_RETRY_LIMIT_REACHED);
					}
				}
				else
				{
					// queue the bundle
					_set.insert(data);
					_queue.push(data);
				}

				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::BundleExpiredEvent &expired = dynamic_cast<const dtn::core::BundleExpiredEvent&>(*evt);

				// delete all matching elements in the queue
				size_t elements = _queue.size();
				for (size_t i = 0; i < elements; i++)
				{
					const RetransmissionData &data = _queue.front();

					if ((dtn::data::BundleID&)data == expired._bundle)
					{
						dtn::net::TransferAbortedEvent::raise(data.destination, data, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
					}
					else
					{
						_queue.push(data);
					}

					_queue.pop();
				}

				return;
			} catch (const std::bad_cast&) { };
		}

		bool RetransmissionExtension::RetransmissionData::operator!=(const RetransmissionData &obj)
		{
			const dtn::data::BundleID &id1 = dynamic_cast<const dtn::data::BundleID&>(obj);
			const dtn::data::BundleID &id2 = dynamic_cast<const dtn::data::BundleID&>(*this);

			if (id1 != id2) return true;
			if (obj.destination != destination) return true;

			return false;
		}

		bool RetransmissionExtension::RetransmissionData::operator==(const RetransmissionData &obj)
		{
			const dtn::data::BundleID &id1 = dynamic_cast<const dtn::data::BundleID&>(obj);
			const dtn::data::BundleID &id2 = dynamic_cast<const dtn::data::BundleID&>(*this);

			if (id1 != id2) return false;
			if (obj.destination != destination) return false;

			return true;
		}

		size_t RetransmissionExtension::RetransmissionData::getCount() const
		{
			return _count;
		}

		size_t RetransmissionExtension::RetransmissionData::getTimestamp() const
		{
			return _timestamp;
		}

		RetransmissionExtension::RetransmissionData& RetransmissionExtension::RetransmissionData::operator++(int)
		{
			_count++;
			_timestamp = dtn::utils::Clock::getTime();
			size_t backoff = pow((float)retry, (int)_count -1);
			_timestamp += backoff;

			return (*this);
		}

		RetransmissionExtension::RetransmissionData& RetransmissionExtension::RetransmissionData::operator++()
		{
			_count++;
			_timestamp = dtn::utils::Clock::getTime();
			size_t backoff = pow((float)retry, (int)_count -1);
			_timestamp += backoff;

			return (*this);
		}

		RetransmissionExtension::RetransmissionData::RetransmissionData(const dtn::data::BundleID &id, dtn::data::EID d, size_t r)
		 : dtn::data::BundleID(id), destination(d), _timestamp(0), _count(0), retry(r)
		{
			(*this)++;
		}

		RetransmissionExtension::RetransmissionData::~RetransmissionData()
		{
		}
	}
}
