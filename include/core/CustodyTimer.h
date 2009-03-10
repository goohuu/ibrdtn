#ifndef CUSTODYTIMER_H_
#define CUSTODYTIMER_H_

#include "data/Bundle.h"
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
			 * Defaultkonstuktor
			 */
			CustodyTimer();

			/**
			 * Konstruktor
			 */
			CustodyTimer(const Bundle &bundle, unsigned int timeout, unsigned int attempt);

			/**
			 * Destruktor
			 */
			~CustodyTimer();

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
