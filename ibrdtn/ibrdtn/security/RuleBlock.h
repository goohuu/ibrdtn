#ifndef __RULE_BLOCK_
#define __RULE_BLOCK_

#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/BundleString.h"
#include "ibrdtn/data/ExtensionBlock.h"

namespace dtn
{
	namespace security
	{
		/**
		This class transports security rules through the DTN. Rules are transported
		as they would stand in a configuration file. Parsing them is the job of the
		receiver.
		*/
		class RuleBlock : public dtn::data::Block
		{
			public:
				class Factory : public dtn::data::ExtensionBlock::Factory
				{
				public:
					Factory() : dtn::data::ExtensionBlock::Factory(RuleBlock::BLOCK_TYPE) {};
					virtual ~Factory() {};
					virtual dtn::data::Block* create();
				};

				/** block type in the range of experimental blocks */
				static const char BLOCK_TYPE = 201;

				/** specifies if this rules apply for incoming or outgoing bundles */
				enum Directions {
					INCOMING,
					OUTGOING,
					BOTH
				};
				/** tells if the rule shall be added or removed */
				enum Actions {
					ADD,
					REMOVE
				};

				/**
				Parameterless constructor for inserting into bundles. The default
				direction is outgoing and the default action is add.
				*/
				RuleBlock();

				/** does nothing */
				virtual ~RuleBlock();

				/**
				 Sets the rule, which is transported
				 @param rule the rule to be transported
				 */
				void setRule(std::string&);

				/**
				 Sets the rule, which is transported
				 @param rule the rule to be transported
				 */
				void setRule(char const *);

				/**
				Returns the saved rule.
				@return the saved rule
				*/
				std::string getRule() const;

				/**
				Set if this rule applies to incoming or outgoing bundles
				@param dir the direction of the bundles
				*/
				void setDirection(Directions);

				/**
				Get the direction to which this rule applies to.
				@return the direction of the bundles
				*/
				Directions getDirection() const;

				/**
				Set if this rule shall be added or removed.
				@param action add or remove the rule
				*/
				void setAction(Actions);

				/**
				Tells if this rule shall be added or removed
				@return ADD if the rule shall be added, and REMOVE if it shall be
				removed
				*/
				Actions getAction() const;

			protected:
				/** the rule, which shall be interpreted by the destination */
				dtn::data::BundleString _rule;
				/** specifies if the rules applies to incoming or outgoing bundles. Will
				be serialized as a SDNV */
				Directions _direction;
				/** wether the configuration class shall add or remove this rule */
				Actions _action;

				/**
				Return the length of the payload, if this were an abstract block. It is
				the length put in the third field, after block type and processing flags.
				@return the length of the payload if this would be an abstract block
				*/
				virtual size_t getLength() const;

				/**
				Serializes this block into a stream.
				@param stream the stream in which should be streamed
				@return the same stream in which was streamed
				*/
				virtual std::ostream &serialize(std::ostream &stream) const;

				/**
				Deserializes this block out of a stream.
				@param stream the stream out of which should be deserialized
				@return the same stream out of which was deserialized
				*/
				virtual std::istream &deserialize(std::istream &stream);
		};

		/**
		 * This creates a static block factory
		 */
		static RuleBlock::Factory __RuleBlockFactory__;
	}
}

#endif /* __RULE_BLOCK_ */
