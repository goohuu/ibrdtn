/*
 * Conditional.h
 *
 *  Created on: 16.07.2009
 *      Author: morgenro
 */

#ifndef WAITFORCONDITIONAL_H_
#define WAITFORCONDITIONAL_H_

#include "ibrdtn/utils/Conditional.h"

namespace dtn
{
	namespace utils
	{
		class WaitForConditional : public dtn::utils::Conditional
		{
		public:
			WaitForConditional() {};
			virtual ~WaitForConditional() {};

			void wait(size_t time = 0)
			{
				enter();
				if (time == 0)
					dtn::utils::Conditional::wait();
				else
					dtn::utils::Conditional::wait(time);
				leave();
			}

			void go()
			{
				enter();
				dtn::utils::Conditional::signal(true);
				leave();
			}
		};
	}
}

#endif /* WAITFORCONDITIONAL_H_ */
