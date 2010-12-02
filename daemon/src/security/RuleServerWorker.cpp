#include "security/RuleServerWorker.h"
#include "Configuration.h"
#include "core/BundleCore.h"

namespace dtn
{
	namespace security
	{

		RuleServerWorker::RuleServerWorker()
		{
			AbstractWorker::initialize(RuleServerWorker::getSuffix(), true);
		}

		RuleServerWorker::~RuleServerWorker()
		{
		}

		void RuleServerWorker::callbackBundleReceived(const Bundle &b)
		{
			dtn::daemon::Configuration::Security& conf = dtn::daemon::Configuration::getInstance().getSecurity();
			// if a RuleBlock is in this bundle, this is a rule for us
			std::list<dtn::security::RuleBlock const *> rbs = b.getBlocks<dtn::security::RuleBlock>();
			for (std::list<dtn::security::RuleBlock const *>::iterator it = rbs.begin(); it != rbs.end(); it++)
			{
				conf.takeRule(**it);
			}
			
			if (rbs.size() == 0)
			{
				// no rule? this is a rule request and we send our rules
				dtn::data::Bundle bundle;
				bundle._source = _eid;
				bundle._destination = b._source;
				std::list<std::string> rules = conf.getSecurityRules_string();
				for (std::list<std::string>::iterator it = rules.begin(); it != rules.end(); it++)
				{
					dtn::security::RuleBlock& rb = bundle.push_back<dtn::security::RuleBlock>();
					// atm only rules for outgoing bundle are supported
					rb.setDirection(dtn::security::RuleBlock::OUTGOING);
					rb.setRule(*it);
				}
				transmit(bundle);
			}
			
			try 
			{
				dtn::core::BundleStorage * stor = &core::BundleCore::getInstance().getStorage();
				if (stor)
				{
					stor->remove(b);
				}
			}
			catch (...) {}
		}

		string RuleServerWorker::getSuffix()
		{
			std::string suffix("/ruleserver");
			return suffix;
		}
	}
}
