#include "net/ConvergenceLayer.h"
#include "net/BundleReceiver.h"

namespace dtn
{
	namespace net
	{
		ConvergenceLayer::Job::Job(const dtn::data::EID &eid, const dtn::data::BundleID &b)
		 : _bundle(b), _destination(eid)
		{
		}

		ConvergenceLayer::Job::~Job()
		{
		}
	}
}
