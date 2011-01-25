/*
 * EpidemicControlMessage.cpp
 *
 *  Created on: 24.01.2011
 *      Author: morgenro
 */

#include "routing/epidemic/EpidemicControlMessage.h"

namespace dtn
{
	namespace routing
	{
		EpidemicControlMessage::EpidemicControlMessage()
		 : type(ECM_QUERY_SUMMARY_VECTOR), flags(0)
		{
		}

		EpidemicControlMessage::~EpidemicControlMessage()
		{
		}

		void EpidemicControlMessage::setSummaryVector(const SummaryVector &vector)
		{
			flags |= ECM_CONTAINS_SUMMARY_VECTOR;
			_vector = vector;
		}

		const SummaryVector& EpidemicControlMessage::getSummaryVector() const
		{
			return _vector;
		}

		void EpidemicControlMessage::setPurgeVector(const SummaryVector &vector)
		{
			flags |= ECM_CONTAINS_PURGE_VECTOR;
			_purge = vector;
		}

		const SummaryVector& EpidemicControlMessage::getPurgeVector() const
		{
			return _purge;
		}

		std::ostream& operator<<(std::ostream &stream, const EpidemicControlMessage &ecm)
		{
			// write the message type
			char mt = ecm.type;
			stream.write((char*)&mt, 1);

			// write the message flags
			char mf = ecm.flags;
			stream.write((char*)&mf, 1);

			if (ecm.type == EpidemicControlMessage::ECM_RESPONSE)
			{
				if (ecm.flags == EpidemicControlMessage::ECM_CONTAINS_SUMMARY_VECTOR)
				{
					stream << ecm._vector;
				}

				if (ecm.flags == EpidemicControlMessage::ECM_CONTAINS_PURGE_VECTOR)
				{
					stream << ecm._purge;
				}
			}

			return stream;
		}

		std::istream& operator>>(std::istream &stream, EpidemicControlMessage &ecm)
		{
			// read the message type
			char mt = 0;
			stream.read((char*)&mt, 1);
			ecm.type = (EpidemicControlMessage::MessageType)mt;

			// read the message flags
			stream.read((char*)&ecm.flags, 1);

			if (ecm.type == EpidemicControlMessage::ECM_RESPONSE)
			{
				if (ecm.flags == EpidemicControlMessage::ECM_CONTAINS_SUMMARY_VECTOR)
				{
					stream >> ecm._vector;
				}

				if (ecm.flags == EpidemicControlMessage::ECM_CONTAINS_PURGE_VECTOR)
				{
					stream >> ecm._purge;
				}
			}

			return stream;
		}
	}
}
