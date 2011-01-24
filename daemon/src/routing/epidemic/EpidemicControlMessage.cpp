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
		{
		}

		EpidemicControlMessage::~EpidemicControlMessage()
		{
		}

		std::ostream& operator<<(std::ostream &stream, const EpidemicControlMessage &ecm)
		{
			return stream;
		}

		std::istream& operator>>(std::istream &stream, EpidemicControlMessage &ecm)
		{
			return stream;
		}
	}
}
