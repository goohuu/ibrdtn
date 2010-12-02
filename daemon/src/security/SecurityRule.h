/*
 * SecurityRule.h
 *
 *  Created on: 02.12.2010
 *      Author: morgenro
 */

#ifndef SECURITYRULE_H_
#define SECURITYRULE_H_

#include <ibrdtn/security/SecurityBlock.h>
#include <ibrdtn/data/EID.h>
#include <list>

namespace dtn
{
	namespace security
	{
		/**
		Specifies which actions have to be taken, when sending a bundle. Each rule
		specifies the destination, to which it applies, and the actions and their
		order which have to be taken.
		*/
		class SecurityRule
		{
		public:
			/**
			Represents one rule. It specifies the type of block, which should be
			added and the destinations
			*/
			class RuleToken
			{
				public:
					/**
					Creates a rule token with type btype
					@param btype the blocktype which should be added
					*/
					RuleToken(const dtn::security::SecurityBlock::BLOCK_TYPES btype) : _type(btype) {}

					/**
					Creates a rule token with type btype and a target list
					@param btype the blocktype which should be added
					@param targets the targets for which a block will be added
					*/
					RuleToken(const dtn::security::SecurityBlock::BLOCK_TYPES btype, const std::list<dtn::data::EID>& targets) : _type(btype), _targets(targets) {}

					/**
					Copy contructor to perform a deep copy.
					*/
					RuleToken(const RuleToken& rt) : _type(rt._type)
					{
						for (std::list<dtn::data::EID>::const_iterator it = rt._targets.begin(); it != rt._targets.end(); it++)
							_targets.push_back(*it);
					}

					/** does nothing */
					virtual ~RuleToken() {}

					/**
					Copies the rt into this instance
					@param rt the token to be copied from
					@return a reference to this instance
					*/
					RuleToken& operator=(const RuleToken& rt)
					{
						_type = rt._type;
						_targets.clear();
						for (std::list<dtn::data::EID>::const_iterator it = rt._targets.begin(); it != rt._targets.end(); it++)
							_targets.push_back(*it);
						return *this;
					}

					/**
					@return the type of the block to be added
					*/
					dtn::security::SecurityBlock::BLOCK_TYPES getType() const { return _type;}

					/**
					@return the targets which are the security destination of this block
					*/
					const std::list<dtn::data::EID>& getTargets() const { return _targets;}
				protected:
					dtn::security::SecurityBlock::BLOCK_TYPES _type;
					/** all nodes for which this token shall be applied at once to a bundle */
					std::list<dtn::data::EID> _targets;
			};

			/**
			 * Creates an empty rule, with no target or rules.
			 */
			SecurityRule();

			/**
			Parses rule and creates a rule from it
			@param rule a string containing the rules, which shall be followed to
			a certain node
			*/
			SecurityRule(const std::string& rule);

			/**
			Copy contructor creates a deep copy
			@param rule to the copied from
			*/
			SecurityRule(const SecurityRule&);

			/** does nothing */
			virtual ~SecurityRule();

			/**
			Copies a rule into this rule
			@param rule to be copied from
			*/
			SecurityRule& operator=(const SecurityRule&);

			/**
			@return the name of the destination node
			*/
			dtn::data::EID getDestination();

			/**
			@return a list of the rules, which shall be followed, before sending
			the bundle. the order of the rules defines the order in which the
			blocks shall be added.
			*/
			const std::list<RuleToken>& getRules() const;

			/**
			@param string a string which may have occurences of deliminiter
			@param deliminiter a char at which the string shall be cut into pieces
			@return a list of substrings, which have been separated by
			delimniniter
			*/
			static std::list<std::string> tokenize(const std::string& string, const char deliminiter);

		protected:
			/** the target node of the bundle */
			dtn::data::EID _destination;
			/** the list of rules, which shall be applied to a bundle targeting _destination */
			std::list<RuleToken> _rules;
		};
	}
}

#endif /* SECURITYRULE_H_ */
