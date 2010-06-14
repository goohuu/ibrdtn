/*
 * CustodyEvent.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "core/CustodyEvent.h"
#include "ibrdtn/data/Exceptions.h"


using namespace dtn::data;
using namespace dtn::core;
using namespace std;

namespace dtn
{
	namespace core
	{
		CustodyEvent::CustodyEvent(const Bundle &bundle, const EventCustodyAction action)
		: m_bundle(bundle), m_action(action)
		{
		}

		CustodyEvent::~CustodyEvent()
		{
		}

		const Bundle& CustodyEvent::getBundle() const
		{
			return m_bundle;
		}

		EventCustodyAction CustodyEvent::getAction() const
		{
			return m_action;
		}

		const string CustodyEvent::getName() const
		{
			return CustodyEvent::className;
		}

		string CustodyEvent::toString() const
		{
			return className;
		}

		const string CustodyEvent::className = "CustodyEvent";
	}
}
