#ifndef BUNDLERECEIVER_H_
#define BUNDLERECEIVER_H_

#include "data/Bundle.h"
#include "core/ConvergenceLayer.h"

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		class BundleReceiver
		{
			public:
			BundleReceiver() {};
			virtual ~BundleReceiver() {};
			virtual void received(ConvergenceLayer *cl, Bundle *b) = 0;
		};
	}
}

#endif /*BUNDLERECEIVER_H_*/
