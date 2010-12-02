#ifndef _KEY_RULE_WORKER_H_
#define _KEY_RULE_WORKER_H_

#include "core/AbstractWorker.h"

namespace dtn
{
	namespace security
	{
		class RuleServerWorker : public dtn::core::AbstractWorker
		{
			public:
				RuleServerWorker();
				virtual ~RuleServerWorker();
				virtual void callbackBundleReceived(const Bundle &b);

				static std::string getSuffix();
		};
	}
}

#endif /* _KEY_RULE_WORKER_H_ */
