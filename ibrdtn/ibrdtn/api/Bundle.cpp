/*
 * Bundle.cpp
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/api/SecurityManager.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Exceptions.h"

namespace dtn
{
	namespace api
	{
		Bundle::Bundle()
		 : _priority(PRIO_MEDIUM)
		{
			_security = SecurityManager::getDefault();
			setPriority(PRIO_MEDIUM);
		}

		Bundle::Bundle(dtn::data::Bundle &b)
		 : _b(b), _priority(PRIO_MEDIUM)
		{
			// read priority
			if (_b._procflags & dtn::data::Bundle::PRIORITY_BIT1)
			{
				if (_b._procflags & dtn::data::Bundle::PRIORITY_BIT2) _priority = PRIO_HIGH;
				else _priority = PRIO_MEDIUM;
			}
			else
			{
				_priority = PRIO_LOW;
			}
		}

		Bundle::Bundle(dtn::data::EID destination)
		 : _priority(PRIO_MEDIUM)
		{
			_b._destination = destination;
			_security = SecurityManager::getDefault();
			setPriority(PRIO_MEDIUM);
		}

		Bundle::~Bundle()
		{
		}

		void Bundle::setLifetime(unsigned int lifetime)
		{
			_lifetime = lifetime;

			// set the lifetime
			_b._lifetime = _lifetime;
		}

		unsigned int Bundle::getLifetime()
		{
			return _lifetime;
		}

		Bundle::BUNDLE_PRIORITY Bundle::getPriority()
		{
			return _priority;
		}

		void Bundle::setPriority(Bundle::BUNDLE_PRIORITY p)
		{
			_priority = p;

			// set the priority to the real bundle
			switch (_priority)
			{
			case PRIO_LOW:
				if (_b._procflags & dtn::data::Bundle::PRIORITY_BIT1) _b._procflags -= dtn::data::Bundle::PRIORITY_BIT1;
				if (_b._procflags & dtn::data::Bundle::PRIORITY_BIT2) _b._procflags -= dtn::data::Bundle::PRIORITY_BIT2;
				break;

			case PRIO_HIGH:
				if (_b._procflags & dtn::data::Bundle::PRIORITY_BIT1) _b._procflags -= dtn::data::Bundle::PRIORITY_BIT1;
				if (!(_b._procflags & dtn::data::Bundle::PRIORITY_BIT2)) _b._procflags += dtn::data::Bundle::PRIORITY_BIT2;
				break;

			case PRIO_MEDIUM:
				if (!(_b._procflags & dtn::data::Bundle::PRIORITY_BIT1)) _b._procflags += dtn::data::Bundle::PRIORITY_BIT1;
				if (_b._procflags & dtn::data::Bundle::PRIORITY_BIT2) _b._procflags -= dtn::data::Bundle::PRIORITY_BIT2;
				break;
			}
		}


		dtn::data::EID Bundle::getDestination()
		{
			return _b._destination;
		}

		dtn::data::EID Bundle::getSource()
		{
			return _b._source;
		}

		ibrcommon::BLOB::Reference Bundle::getData()
		{
			try {
				dtn::data::PayloadBlock &p = _b.getBlock<dtn::data::PayloadBlock>();
				return p.getBLOB();
			} catch(dtn::data::Bundle::NoSuchBlockFoundException ex) {
				throw dtn::MissingObjectException("No payload block exists!");
			}
		}
	}
}

