/*
 * Bundle.cpp
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/Exceptions.h"

namespace dtn
{
	namespace api
	{
		Bundle::Bundle()
		{
			setPriority(PRIO_MEDIUM);

			_b._source = dtn::data::EID("api:me");
		}

		Bundle::Bundle(const dtn::data::Bundle &b)
		 : _b(b)
		{
		}

		Bundle::Bundle(const dtn::data::EID &destination)
		{
			setDestination(destination);
			setPriority(PRIO_MEDIUM);
		}

		Bundle::~Bundle()
		{
		}

		void Bundle::setLifetime(unsigned int lifetime)
		{
			_b._lifetime = lifetime;
		}

		unsigned int Bundle::getLifetime() const
		{
			return _b._lifetime;
		}

		void Bundle::requestDeliveredReport()
		{
			_b.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_DELIVERY, true);
		}

		void Bundle::requestForwardedReport()
		{
			_b.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_FORWARDING, true);
		}

		void Bundle::requestDeletedReport()
		{
			_b.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_DELETION, true);
		}

		void Bundle::requestReceptionReport()
		{
			_b.set(dtn::data::PrimaryBlock::REQUEST_REPORT_OF_BUNDLE_RECEPTION, true);
		}

		void Bundle::requestEncryption()
		{
			_b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT, true);
		}

		void Bundle::requestSigned()
		{
			_b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN, true);
		}

		bool Bundle::statusVerified()
		{
			return _b.get(dtn::data::PrimaryBlock::DTNSEC_STATUS_VERIFIED);
		}

		Bundle::BUNDLE_PRIORITY Bundle::getPriority() const
		{
			// If none of the priority bit are set, then the priority is LOW. bit1 = 1 and
			// bit2 = 0 indicated an MEDIUM priority and both bits set leads to a HIGH
			// priority.
			if (_b.get(dtn::data::Bundle::PRIORITY_BIT1))
			{
					if (_b.get(dtn::data::Bundle::PRIORITY_BIT2))
					{
						return PRIO_HIGH;
					}

					return PRIO_MEDIUM;
			}

			return PRIO_LOW;
		}

		void Bundle::setPriority(Bundle::BUNDLE_PRIORITY p)
		{
			// set the priority to the real bundle
			switch (p)
			{
			case PRIO_LOW:
				_b.set(dtn::data::Bundle::PRIORITY_BIT1, false);
				_b.set(dtn::data::Bundle::PRIORITY_BIT2, false);
				break;

			case PRIO_HIGH:
				_b.set(dtn::data::Bundle::PRIORITY_BIT1, false);
				_b.set(dtn::data::Bundle::PRIORITY_BIT2, true);
				break;

			case PRIO_MEDIUM:
				_b.set(dtn::data::Bundle::PRIORITY_BIT1, true);
				_b.set(dtn::data::Bundle::PRIORITY_BIT2, false);
				break;
			}
		}

		void Bundle::setDestination(const dtn::data::EID &eid, const bool singleton)
		{
			if (singleton)
				_b.set(dtn::data::Bundle::DESTINATION_IS_SINGLETON, true);
			else
				_b.set(dtn::data::Bundle::DESTINATION_IS_SINGLETON, false);

			_b._destination = eid;
		}

		void Bundle::setReportTo(const dtn::data::EID &eid)
		{
			_b._reportto = eid;
		}

		dtn::data::EID Bundle::getReportTo() const
		{
			return _b._reportto;
		}

		dtn::data::EID Bundle::getDestination() const
		{
			return _b._destination;
		}

		dtn::data::EID Bundle::getSource() const
		{
			return _b._source;
		}

		dtn::data::EID Bundle::getCustodian() const
		{
			return _b._custodian;
		}

		ibrcommon::BLOB::Reference Bundle::getData() throw (dtn::MissingObjectException)
		{
			try {
				dtn::data::PayloadBlock &p = _b.getBlock<dtn::data::PayloadBlock>();
				return p.getBLOB();
			} catch(dtn::data::Bundle::NoSuchBlockFoundException ex) {
				throw dtn::MissingObjectException("No payload block exists!");
			}
		}

		bool Bundle::operator<(const Bundle& other) const
		{
			return (_b < other._b);
		}

		bool Bundle::operator>(const Bundle& other) const
		{
			return (_b > other._b);
		}
	}
}
