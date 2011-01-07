#ifndef _EXTENSION_SECURITY_BLOCK_H_
#define _EXTENSION_SECURITY_BLOCK_H_
#include "ibrdtn/security/SecurityBlock.h"
#include "ibrdtn/data/ExtensionBlock.h"
#include <ibrcommon/ssl/AES128Stream.h>

namespace dtn
{
	namespace security
	{
		/**
		Encrypts ExtensionBlocks and replaces them with an ExtensionSecurityBlock,
		which stores the ciphertext in its security result, which means that there 
		should not be a payloadblock encrypted. You can instantiate a factory of 
		this class with a rsa key and the node belonging to this key. Encryption is 
		done with AES128. The AES-Key will be encrypted using the rsa key and placed
		into the security parameters.\n
		You can encrypt one or a series of blocks using the encryptBlock() method.
		Encryption is done with the decryptBlock() method.\n
		Take care which kind of rsa key is given to this class. You can instantiate 
		it with a public rsa key, but decryption will fail with it and you only 
		notice it, when your programm breaks.
		*/
		class ExtensionSecurityBlock : public SecurityBlock
		{
			/**
			This class is allowed to call the parameterless contructor and the 
			constructor with the bundle parameter.
			*/
			friend class dtn::data::Bundle;
				public:
				class Factory : public dtn::data::ExtensionBlock::Factory
				{
				public:
					Factory() : dtn::data::ExtensionBlock::Factory(ExtensionSecurityBlock::BLOCK_TYPE) {};
					virtual ~Factory() {};
					virtual dtn::data::Block* create();
				};

				/** The block type of this class. */
				static const char BLOCK_TYPE = SecurityBlock::EXTENSION_SECURITY_BLOCK;

				/**
				Creates an ExtensionSecurityBlock-Factory, with long_key as a private or
				public key. Note that a factory can only be used either for encryption
				or decryption.
				@param long_key the rsa key to be used
				@param we the EID of this node
				@param partner the EID of the node with the private key, if a public key
				is used
				*/
				ExtensionSecurityBlock(RSA * long_key);

				/** does nothing */
				virtual ~ExtensionSecurityBlock();

				/**
				Encrypts and encapsulates a block into a ExtensionSecurityBlock. The 
				ExtensionSecurityBlock will be placed at the same place as the original
				block.
				@param bundle the bundle to which block belongs
				@param block the to be encrypted block
				*/
				void encryptBlock(dtn::data::Bundle& bundle, const dtn::data::Block * const block, const dtn::data::EID& source, const dtn::data::EID& destination) const;

				/**
				Encrypts and encapsulates two or more blocks into two or more
				ExtensionSecurityBlocks. They are correlated together and only the first
				block carries key information. Each block is replaced by an 
				ExtensionSecurityBlock carrying the block inside encrypted.
				@param bundle the bundle to which the blocks belong
				@param blocks the to be encrypted blocks
				*/
				void encryptBlock(dtn::data::Bundle& bundle, const std::list<dtn::data::Block const *>& blocks, const dtn::data::EID& source, const dtn::data::EID& destination) const;

				/**
				Decrypts all Blocks in this bundle, for which we have the matching key. 
				There might be other blocks, which can't be encrypted and will remain
				untouched. The only blocks on which decryption is attempted are such, 
				which contain this node as security destination, or if security
				destination is not set, this node has to be the destination. If there 
				are correlated blocks, care is taken to decrypt them properly.
				@param bundle the bundle with ESBs to be decrypted
				@return true if block decryption succeeded and all blocks have been 
				replaced, false otherwise. when false the blocks where decryption failed
				will remain as they were.
				*/
				bool decryptBlock(dtn::data::Bundle& bundle) const;

				/**
				Decrypts the given block and replaces the ESB with the original block in
				the bundle. This block must carry the symmetric AES key, which was used 
				to decrypt, and not be correlated.
				@param bundle the bundle to which block belongs
				@param block the to be decrypted block
				@return true if block decryption succeeded and the block has been 
				replaced, false otherwise. when false the encrypted block will remain as
				it was
				*/
				bool decryptBlock(dtn::data::Bundle& bundle, dtn::security::ExtensionSecurityBlock const * block) const;

				/**
				Decrypts  the given blocks in the bundle assuming, that they all
				are correlated. The first block must carry the key information needed to
				decrypt the other blocks.
				@param bundle the bundle to which blocks belong
				@param blocks the to be decrypted blocks
				@return true if block decryption succeeded and the blocks have been 
				replaced, false otherwise. when false the old blocks will remain intakt.
				But be carefull. If from a series of blocks one block fails, this single
				block will remain encrypted inside the bundle. the other blocks will 
				decrypted, so recovering the failed block may be impossible.
				*/
				bool decryptBlock(dtn::data::Bundle& bundle, std::list<dtn::security::ExtensionSecurityBlock const *>& blocks) const;

				/**
				Decrypts all blocks in the bundle which have correlator as their
				correlatorvalue set. Assuming that they belong together, with
				first block carrying the key information.
				@param bundle the bundle to which the blocks belong to
				@param correlator the correlator which have the blocks set
				@return true if block decryption succeeded and the blocks have been 
				replaced, false otherwise. when false the old blocks will remain intakt.
				But be carefull. If from a series of blocks one block fails, this single
				block will remain encrypted inside the bundle. the other blocks will 
				decrypted, so recovering the failed block may be impossible.
				*/
				bool decryptBlocks(dtn::data::Bundle& bundle, u_int64_t correlator) const;

			protected:
				/**
				Creates an empty ExtensionSecurityBlock and sets its ciphersuite id to
				ESB_RSA_AES128_EXT
				*/
				ExtensionSecurityBlock();

				/**
				Creates an empty ExtensionSecurityBlock and sets its ciphersuite id to
				ESB_RSA_AES128_EXT
				@param bundle not used parameter. this is needed for the insert()-Method
				of the bundle class, because it expects all classes in instantiates to 
				have this constructor.
				*/
				ExtensionSecurityBlock(const dtn::data::Bundle& bundle);
				
			private:
				/**
				Pointer to the rsa key used for decryption or encryption
				*/
				RSA * _rsa;
		};

		/**
		 * This creates a static block factory
		 */
		static ExtensionSecurityBlock::Factory __ExtensionSecurityBlockFactory__;
	}
}

#endif
