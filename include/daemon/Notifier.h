/*
 * Notifier.h
 *
 *  Created on: 11.12.2009
 *      Author: morgenro
 */

#ifndef NOTIFIER_H_
#define NOTIFIER_H_

#include "ibrdtn/default.h"
#include "core/EventReceiver.h"
#include <iostream>

namespace dtn
{
	namespace daemon
	{
		class Notifier : public dtn::core::EventReceiver
		{
		public:
			Notifier(std::string cmd);
			virtual ~Notifier();

			void raiseEvent(const dtn::core::Event *evt);

			void notify(string title, string msg);

		private:
			string _cmd;
		};
	}
}

#endif /* NOTIFIER_H_ */
