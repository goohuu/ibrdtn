#ifndef BUNDLESCHEDULE_H_
#define BUNDLESCHEDULE_H_

#include "data/Bundle.h"

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * A bundle schedule holds some information about the next route
		 * for a bundle.
		 */
		class BundleSchedule
		{
			public:
				BundleSchedule();
				BundleSchedule(const Bundle &b, unsigned int dtntime, string eid);
				BundleSchedule(const BundleSchedule &k);

				~BundleSchedule();

				const Bundle& getBundle() const;
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
