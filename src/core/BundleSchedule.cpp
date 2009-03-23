#include "core/BundleSchedule.h"
#include <limits.h>

namespace dtn
{
	namespace core
	{
		const unsigned int BundleSchedule::MAX_TIME = UINT_MAX;

		BundleSchedule::BundleSchedule()
		: m_bundle(NULL), m_time(0), m_eid("dtn:none")
		{
		}

		BundleSchedule::BundleSchedule(const Bundle &b, unsigned int dtntime, string eid)
		: m_bundle(NULL), m_time(dtntime), m_eid(eid)
		{
			m_bundle = new Bundle(b);
		}

		BundleSchedule::BundleSchedule(const BundleSchedule &k)
		: m_bundle(NULL), m_time(k.m_time), m_eid(k.m_eid)
		{
			m_bundle = new Bundle(*k.m_bundle);
		}

		BundleSchedule::~BundleSchedule()
		{
			if (m_bundle != NULL) delete m_bundle;
		}

		unsigned int BundleSchedule::getTime() const
		{
			return m_time;
		}

		string BundleSchedule::getEID() const
		{
			return m_eid;
		}

		const Bundle& BundleSchedule::getBundle() const
		{
			if (m_bundle == NULL) throw dtn::exceptions::MissingObjectException();

			return *m_bundle;
		}
	}
}
