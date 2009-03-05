/*
 * PositionEvent.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef POSITIONEVENT_H_
#define POSITIONEVENT_H_

#include <string>
#include "core/Node.h"
#include "core/Event.h"

using namespace dtn::core;
using namespace std;

namespace emma
{
	class PositionEvent : public Event
	{
	public:
		PositionEvent(const pair<double,double> position);
		~PositionEvent();

		pair<double,double> getPosition() const;

		const string getName() const;

#ifdef DO_DEBUG_OUTPUT
		string toString();
#endif

		static const string className;

	private:
		const pair<double,double> &m_position;
	};
}

#endif /* POSITIONEVENT_H_ */
