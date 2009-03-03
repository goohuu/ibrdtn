#include "core/BundleSchedule.h"
#include <limits.h>

namespace dtn
{
namespace core
{
	const unsigned int BundleSchedule::MAX_TIME = UINT_MAX;
	
	BundleSchedule::BundleSchedule(Bundle *b, unsigned int dtntime, string eid)
	{
		m_bundle = b;
		m_time = dtntime;
		m_eid = eid;
	}
	
	unsigned int BundleSchedule::getTime()
	{
		return m_time;
	}
	
	string BundleSchedule::getEID()
	{
		return m_eid;
	}
	
	Bundle* BundleSchedule::getBundle()
	{
		return m_bundle;
	}
		
}
}
