#include "ibrdtn/security/RuleBlock.h"
#include "ibrdtn/data/SDNV.h"

namespace dtn
{
	namespace security
	{
		dtn::data::Block* RuleBlock::Factory::create()
		{
			return new RuleBlock();
		}

		RuleBlock::RuleBlock()
		: Block(BLOCK_TYPE), _direction(OUTGOING), _action(ADD)
		{
		}

		RuleBlock::~RuleBlock()
		{
		}

		void RuleBlock::setRule(const std::string& rule)
		{
			_rule = dtn::data::BundleString(rule);
		}

		void RuleBlock::setRule(const char * rule)
		{
			std::string rule_string(rule);
			setRule(rule_string);
		}

		std::string RuleBlock::getRule() const
		{
			return _rule;
		}

		void RuleBlock::setDirection(Directions dir)
		{
			_direction = dir;
		}

		RuleBlock::Directions RuleBlock::getDirection() const
		{
			return _direction;
		}

		void RuleBlock::setAction(Actions action)
		{
			_action = action;
		}

		RuleBlock::Actions RuleBlock::getAction() const
		{
			return _action;
		}

		size_t RuleBlock::getLength() const
		{
			return dtn::data::SDNV::getLength(_direction) + dtn::data::SDNV::getLength(_action) + _rule.getLength();
		}

		std::ostream& RuleBlock::serialize(std::ostream &stream) const
		{
			stream << dtn::data::SDNV(_direction) << dtn::data::SDNV(_action) << dtn::data::BundleString(_rule);
			return stream;
		}

		std::istream& RuleBlock::deserialize(std::istream &stream)
		{
			dtn::data::SDNV dir, act;
			dtn::data::BundleString rule_bstring;
			stream >> dir >> act >> rule_bstring;
			_direction = static_cast<Directions>(dir.getValue());
			_action = static_cast<Actions>(act.getValue());
			_rule = rule_bstring;
			return stream;
		}
	}
}
