/*
 * PositonEvent.cpp
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "emma/PositionEvent.h"

using namespace dtn::core;
using namespace std;

namespace emma
{
	PositionEvent::PositionEvent(const pair<double,double> position) : m_position(position)
	{}

	PositionEvent::~PositionEvent()
	{}

	pair<double,double> PositionEvent::getPosition() const
	{
		return m_position;
	}

	const string PositionEvent::getName() const
	{
		return PositionEvent::className;
	}

#ifdef DO_DEBUG_OUTPUT
	string PositionEvent::toString()
	{
		return className;
	}
#endif

	const string PositionEvent::className = "PositionEvent";
}
