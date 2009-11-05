#include "core/CustodyTimer.h"


namespace dtn
{
	namespace core
	{
		CustodyTimer::CustodyTimer()
		 : m_time(0), m_attempt(0)
		{
		}

		/**
		 * Konstruktor
		 */
		CustodyTimer::CustodyTimer(const Bundle &bundle, unsigned int timeout, unsigned int attempt)
		 : m_bundle(bundle), m_time(timeout), m_attempt(attempt)
		{
		}

		/**
		 * destructor
		 */
		CustodyTimer::~CustodyTimer()
		{
		}

		unsigned int CustodyTimer::getTime() const
		{
			return m_time;
		}

		const Bundle& CustodyTimer::getBundle() const
		{
			return m_bundle;
		}

		unsigned int CustodyTimer::getAttempt() const
		{
			return m_attempt;
		}
	}
}
