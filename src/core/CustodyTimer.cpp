#include "core/CustodyTimer.h"
#include "data/BundleFactory.h"

namespace dtn
{
namespace core
{
	CustodyTimer::CustodyTimer()
	 : m_bundle(NULL), m_node(PERMANENT), m_time(0), m_attempt(0)
	{
	}
	
	/**
	 * Konstruktor
	 */
	CustodyTimer::CustodyTimer(Node node, Bundle *bundle, unsigned int timeout, unsigned int attempt)
	 : m_bundle(bundle), m_node(node), m_time(timeout), m_attempt(attempt)
	{
	}
	
	/**
	 * Destruktor
	 */
	CustodyTimer::~CustodyTimer()
	{
	}

	unsigned int CustodyTimer::getTime() const
	{
		return m_time;
	}
	
	Bundle* CustodyTimer::getBundle() const
	{
		return m_bundle;
	}
	
	const Node& CustodyTimer::getCustodyNode() const
	{
		return m_node;
	}	
	
	unsigned int CustodyTimer::getAttempt() const
	{
		return m_attempt;
	}
}
}
