/*
 * SecurityRule.cpp
 *
 *  Created on: 02.12.2010
 *      Author: morgenro
 */

#include "SecurityRule.h"

namespace dtn
{
	namespace security
	{
		SecurityRule::SecurityRule()
		 : _destination(dtn::data::EID())
		{
		}

		SecurityRule::SecurityRule(const std::string& rule)
		{
			std::list<std::string> tokens = tokenize(rule, ';');

#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(tokens.size() >= 2);
#endif

			std::list<std::string>::iterator it = tokens.begin();
			_destination = dtn::data::EID(*it);
			it++;

			for (; it != tokens.end(); it++)
			{
				if (*it == "PIB" || *it == "pib")
					_rules.push_back(RuleToken(SecurityBlock::PAYLOAD_INTEGRITY_BLOCK));
				else if (it->substr(0,3) == "PCB" || it->substr(0,3) == "pcb")
				{
					std::list<std::string> targets = tokenize(*it, '|');
					std::list<dtn::data::EID> eid_targets;
					std::list<std::string>::iterator target_it = targets.begin();
					target_it++;
					for (;target_it != targets.end(); target_it++)
						eid_targets.push_back(dtn::data::EID(*target_it));
					_rules.push_back(RuleToken(SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK, eid_targets));
				}
				else if (it->substr(0,3) == "ESB" || it->substr(0,3) == "esb")
					_rules.push_back(RuleToken(SecurityBlock::EXTENSION_SECURITY_BLOCK));
			}
		}

		SecurityRule::SecurityRule(const dtn::security::SecurityRule& rule)
		{
			*this = rule;
		}

		SecurityRule::~SecurityRule()
		{
		}

		SecurityRule& SecurityRule::operator=(const dtn::security::SecurityRule& rule)
		{
			_destination = rule._destination;
			_rules.clear();
			for (std::list<SecurityRule::RuleToken >::const_iterator it = rule._rules.begin(); it != rule._rules.end(); it++)
			{
				_rules.push_back(*it);
			}
			return *this;
		}

		dtn::data::EID SecurityRule::getDestination()
		{
			return _destination;
		}

		const std::list< SecurityRule::RuleToken >& SecurityRule::getRules() const
		{
			return _rules;
		}

		std::list< string > SecurityRule::tokenize(const std::string& string, const char deliminiter)
		{
			size_t pos = 0;
			std::list<std::string> tokens;
			while (pos != std::string::npos)
			{
				size_t next_pos = string.find(deliminiter, pos);

#ifdef __DEVELOPMENT_ASSERTIONS__
				assert(string[next_pos] == deliminiter || next_pos == std::string::npos);
#endif

				tokens.push_back(string.substr(pos, next_pos-pos));

				pos = next_pos;
				if (pos != std::string::npos)
					pos++;
			}
			return tokens;
		}
	}

}
