#ifndef _EXTENSION_SECURITY_BLOCK_H_
#define _EXTENSION_SECURITY_BLOCK_H_
#include "ibrdtn/security/SecurityBlock.h"
#include "ibrdtn/security/SecurityKey.h"
#include "ibrdtn/data/ExtensionBlock.h"

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

				/** does nothing */
				virtual ~ExtensionSecurityBlock();

				/**
				Encrypts and encapsulates a block into a ExtensionSecurityBlock. The 
				ExtensionSecurityBlock will be placed at the same place as the original
				block.
				@param bundle the bundle to which block belongs
				@param block the to be encrypted block
				*/
				static void encrypt(dtn::data::Bundle& bundle, const SecurityKey &key, const dtn::data::Block &block, const dtn::data::EID& source, const dtn::data::EID& destination);

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
				static void decrypt(dtn::data::Bundle& bundle, const SecurityKey &key, const dtn::security::ExtensionSecurityBlock &block);

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
				static void decrypt(dtn::data::Bundle& bundle, const SecurityKey &key, u_int64_t correlator = 0);

			protected:
				/**
				Creates an empty ExtensionSecurityBlock and sets its ciphersuite id to
				ESB_RSA_AES128_EXT
				*/
				ExtensionSecurityBlock();
		};

		/**
		 * This creates a static block factory
		 */
		static ExtensionSecurityBlock::Factory __ExtensionSecurityBlockFactory__;
	}
}

#endif
