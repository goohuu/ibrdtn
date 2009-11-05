#ifndef CUSTODYTIMER_H_
#define CUSTODYTIMER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Bundle.h"
#include "core/Node.h"

using namespace dtn::data;

namespace dtn
{
namespace core
{
	class CustodyTimer
	{
		public:
			/**
			 * default constructor
			 */
			CustodyTimer();

			/**
			 * constructor with a given bundle, timeout and attempt of retransmit.
			 */
			CustodyTimer(const Bundle &bundle, unsigned int timeout, unsigned int attempt);

			/**
			 * destructor
			 */
			virtual ~CustodyTimer();

			unsigned int getTime() const;

			const Bundle& getBundle() const;

			unsigned int getAttempt() const;

		private:
			Bundle m_bundle;
			unsigned int m_time;
			unsigned int m_attempt;
	};
}
}

#endif /*CUSTODYTIMER_H_*/
