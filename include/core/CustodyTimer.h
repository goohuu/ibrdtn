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
			CustodyTimer(Node node, Bundle *bundle, unsigned int timeout, unsigned int attempt);

			/**
			 * Destruktor
			 */
			~CustodyTimer();

			unsigned int getTime() const;

			Bundle* getBundle() const;

			const Node& getCustodyNode() const;

			unsigned int getAttempt() const;

		private:
			Bundle *m_bundle;
			Node m_node;
			unsigned int m_time;
			unsigned int m_attempt;
	};
}
}

#endif /*CUSTODYTIMER_H_*/
