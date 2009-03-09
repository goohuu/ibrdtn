#include "core/BundleSchedule.h"
#include <limits.h>

namespace dtn
{
namespace core
{
	const unsigned int BundleSchedule::MAX_TIME = UINT_MAX;

	BundleSchedule::BundleSchedule(Bundle *b, unsigned int dtntime, string eid)
	: m_bundle(b), m_time(dtntime), m_eid(eid)
	{
	}

	BundleSchedule::BundleSchedule(const BundleSchedule &k)
	: m_bundle(k.m_bundle), m_time(k.m_time), m_eid(k.m_eid)
	{
		//m_bundle = new Bundle(*k.m_bundle);
	}

	unsigned int BundleSchedule::getTime() const
	{
		return m_time;
	}

	string BundleSchedule::getEID() const
	{
		return m_eid;
	}

	Bundle* BundleSchedule::getBundle() const
	{
		return m_bundle;
	}

}
}
