#ifndef _KEY_BLOCK_H__
#define _KEY_BLOCK_H__

#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/BundleString.h"
#include "ibrdtn/security/SecurityBlock.h"
#include "ibrdtn/data/ExtensionBlock.h"
#include <openssl/rsa.h>

namespace dtn
{
	namespace security
	{
		/**
		This class is used for requesting and sending public keys from the network.
		TODO This could be extended to carry certificates as well.
		*/
		class KeyBlock : public dtn::data::Block
		{
			public:
				class Factory : public dtn::data::ExtensionBlock::Factory
				{
				public:
					Factory() : dtn::data::ExtensionBlock::Factory(KeyBlock::BLOCK_TYPE) {};
					virtual ~Factory() {};
					virtual dtn::data::Block* create();
				};

				/** block type in the range of experimental blocks */
				static const char BLOCK_TYPE = 200;

				/**
				Parameterless constructor for inserting into bundles.
				*/
				KeyBlock();

				/** does nothing */
				virtual ~KeyBlock();

				/**
				Sets the EID of the node from which a key is requested
				@param target the node of the requestet key
				*/
				void setTarget(const dtn::data::EID& target);

				/**
				@return the EID of the target node
				*/
				const dtn::data::EID getTarget() const;

				/**
				Sets the block type of the requested key
				@param type the block type of the requested key
				*/
				void setSecurityBlockType(SecurityBlock::BLOCK_TYPES type);

				/**
				@return the type of the security block, after which the key is searched.
				*/
				SecurityBlock::BLOCK_TYPES getSecurityBlockType() const;

				/**
				@return true if a key is set, false if not
				*/
				bool iskeyPresent() const;

				/**
				The rsa object will be added as key to this block
				@param rsa the key, which belongs to the requested node
				*/
				void addKey(RSA * rsa);

				/**
				Returns a copy to the saved key. Do not forget to delete it, if not
				needed!
				@return the saved key
				*/
				RSA * getKey_rsa() const;

				/**
				 * Reads a key in DER format and creates a RSA object
				 * @param key the key in DER format
				 * @return the rsa object constructed from key. the rsa object has to be
				 * deleted by the caller.
				 */
				static RSA * createRSA(const std::string& key);

				/**
				Returns a copy to the saved key. Do not forget to delete it, if not
				needed!
				@return the saved key
				*/
				const std::string& getKey() const;

				/**
				Tests if this equals kb
				@param kb the keyblock which shall be tested for equality
				@return true if this and kb are equal, false otherwise
				*/
				bool operator==(KeyBlock& kb) const;

				/**
				Tests if this equals kb
				@param kb the keyblock which shall be tested for equality
				@return true if this and kb are equal, false otherwise
				*/
				bool operator==(const KeyBlock& kb) const;

				/**
				 * Sets the number of hops this KeyBlock may go through the net until
				 * the requesting node will be asked, if it has received the key
				 * @param times the number of hops
				 */
				void setDistance(unsigned int);

				/**
				 * Returns the number of hops this keyrequest shall travel without refreshing.
				 * @return the number of hops to travel
				 */
				unsigned int getDistance() const;

				/**
				 * True if the the request can be send to other nodes and need not be
				 * refreshed, false otherwise.
				 * @return true if sendable, false if not
				 */
				bool isSendable() const;

				/**
				 * True if the counter needs to be reset, false otherwise
				 * @return true if the counter needs to be refreshed
				 */
				bool isNeedingRefreshing() const;

				/**
				 * Set if the counter needs to be reset
				 * @param status true if the counter shall be reset
				 */
				void setRefresh(bool);


			protected:
				/** the serialized key in pem format is stored here */
				dtn::data::BundleString _key;
				/** the block type for the key */
				SecurityBlock::BLOCK_TYPES _type;
				/** TTL value for refreshing the key request */
				unsigned int _sendable_times;
				bool _refresh_counter;

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
		static KeyBlock::Factory __KeyBlockFactory__;
	}
}

#endif /* _KEY_BLOCK_H__ */
