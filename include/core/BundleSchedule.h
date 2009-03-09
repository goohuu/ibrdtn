#ifndef BUNDLESCHEDULE_H_
#define BUNDLESCHEDULE_H_

#include "data/Bundle.h"

using namespace dtn::data;

namespace dtn
{
namespace core
{
	/**
	 * Ein BundleSchedule ist ein Zeitplan für ein Bundle. Mit ihm wird festgelegt,
	 * wann ein Bundle zu wem versendet werden soll.
	 */
	class BundleSchedule
	{
		public:
			BundleSchedule(Bundle *b, unsigned int dtntime, string eid);
			BundleSchedule(const BundleSchedule &k);

			Bundle* getBundle() const;
			unsigned int getTime() const;
			string getEID() const;

			static const unsigned int MAX_TIME;

		private:
			string m_eid;
			unsigned int m_time;
			Bundle *m_bundle;
	};
}
}



#endif /*BUNDLESCHEDULE_H_*/
