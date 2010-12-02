#ifndef _PAYLOAD_CONFIDENTIAL_BLOCK_H_
#define _PAYLOAD_CONFIDENTIAL_BLOCK_H_
#include "ibrdtn/security/SecurityBlock.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/ExtensionBlock.h"
#include <openssl/rsa.h>

namespace dtn
{
	namespace security
	{
		/**
		The PayloadConfidentialBlock encrypts the payload, PayloadConfidentialBlocks,
		which are already there and PayloadIntegrityBlocks, which are already there.
		Payload Confidential or Integrity Blocks are encrypted because they can 
		contain e.g. signatures which make guessing the plaintext easier. You can 
		instantiate a factory, which will take care of everything. The factory can 
		be given a rsa key and the corresponding node. You may wish to add more keys
		using addDestination(), so one or more nodes are able to recover the 
		payload. For each destination a PayloadConfidentialBlock is placed in the
		bundle, when calling encrypt(). Be sure, that no other 
		PayloadConfidentialBlocks or PayloadIntegrityBlocks are inside this bundle 
		if using encryption with more than one key.
		*/
		class PayloadConfidentialBlock : public SecurityBlock
		{
			/**
			This class is allowed to call the parameterless contructor and the 
			constructor with a bundle as argument.
			*/
			friend class dtn::data::Bundle;
			public:
				class Factory : public dtn::data::ExtensionBlock::Factory
				{
				public:
					Factory() : dtn::data::ExtensionBlock::Factory(PayloadConfidentialBlock::BLOCK_TYPE) {};
					virtual ~Factory() {};
					virtual dtn::data::Block* create();
				};

				/** The block type of this class. */
				static const char BLOCK_TYPE = SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK;

				/**
				Creates a PayloadConfidentialBlock factory, which will add
				PayloadConfidentialBlocks to Bundles and encrypts the payload or
				verifies them and decrypts the payload. A PayloadConfidentialBlock,
				which is used for encryption cannot be used for decryption, because it
				has another type of RSA key set.
				@param long_key the rsa key with which the hashes will be signed or
				verified
				@param we the EID of this node
				@param partner the EID of the node to which the public key belongs to
				*/
				PayloadConfidentialBlock(RSA * long_key, const dtn::data::EID& we, const dtn::data::EID& partner = dtn::data::EID());
				
				/** does nothing */
				virtual ~PayloadConfidentialBlock();
				
				/**
				Adds another node and its key to the factory, which shall be able to 
				decrypt the bundle.  Be sure, that no other PayloadConfidentialBlocks or
				PayloadIntegrityBlocks are inside this bundle if using encryption with 
				more than one key.
				*/
				void addDestination(RSA * long_key, const dtn::data::EID& partner);

				/**
				Encrypts the Payload inside this Bundle. If PIBs or PCBs are found, they
				will be encrypted, too, with a correlator set. The encrypted blocks will
				be each placed inside a PayloadConfidentialBlock, which will be inserted
				at the same place, except for the payload, which be encrypted in place.
				@param bundle the bundle with the to be encrypted payload
				*/
				void encrypt(dtn::data::Bundle& bundle) const;

				/**
				Decrypts the Payload inside this Bundle. All correlated Blocks, which
				are found, will be decrypted, too, placed at the position, where their 
				PayloadConfidentialBlock was, which contained them. After a matching 
				PayloadConfidentialBlock with key information is searched by looking 
				after the security destination. If the payload has been decrypted 
				successfully, the correlated blocks will be decrypted. If one block 
				fails to decrypt, it will be deleted.
				@param bundle the bundle with the to be decrypted payload
				@return true if decryption has been successfull, false otherwise
				*/
				bool decrypt(dtn::data::Bundle& bundle) const;
			protected:
				/**
				Creates an empty PayloadConfidentialBlock. With ciphersuite_id set to
				PCB_RSA_AES128_PAYLOAD_PIB_PCB
				*/
				PayloadConfidentialBlock();

				/**
				Creates an empty PayloadConfidentialBlock. With ciphersuite_id set to
				PCB_RSA_AES128_PAYLOAD_PIB_PCB
				@param bundle this is only needed for the insert()-Methode in bundle and
				not used here
				*/
				PayloadConfidentialBlock(dtn::data::Bundle& bundle);
				
				/**
				Encrypts the payload in place and adds a PCB after the primary block to 
				the bundle. Ephemeral_key will be encrypted and together with salt be 
				placed in the first PayloadConfidentialBlock, of each RSA-Node-Pair.
				@param bundle the bundle containing the payload
				@param ephemeral_key the symmetric AES key
				@param salt the salt
				*/
				void encryptPayload(dtn::data::Bundle& bundle, const unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes], const u_int32_t salt) const;

				/**
				Decrypts the payload using the ephemeral_key and salt.
				@param bundle the payload containing bundle
				@param ephemeral_key the AES key
				@param salt the salt
				@return true if tag verification succeeded, false otherwise
				*/
				bool decryptPayload(dtn::data::Bundle& bundle, const unsigned char ephemeral_key[ibrcommon::AES128Stream::key_size_in_bytes], const u_int32_t salt) const;

			private:
				/** a map of each node with its public key */
				std::map<dtn::data::EID, RSA *> _node_keys;
		};

		/**
		 * This creates a static block factory
		 */
		static PayloadConfidentialBlock::Factory __PayloadConfidentialBlockFactory__;
	}
}

#endif
