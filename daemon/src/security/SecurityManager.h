#ifndef _SECURITY_MANAGER_H_
#define _SECURITY_MANAGER_H_

#include "Configuration.h"
#include "security/KeyServerWorker.h"
#include "security/SecurityRule.h"

#include <ibrdtn/data/EID.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/security/BundleAuthenticationBlock.h>
#include <ibrdtn/security/PayloadIntegrityBlock.h>
#include <ibrdtn/security/PayloadConfidentialBlock.h>
#include <ibrdtn/security/ExtensionSecurityBlock.h>
#include <ibrdtn/security/KeyBlock.h>
#include <map>

namespace dtn
{
	namespace security
	{
		/**
		 * Decrypts or encrypts and signs or verifies bundles, which go in or out.
		 * The rules are read from the configuration and the keys needed for
		 * operation must be in the same directory as the configuration or be
		 * retrievable from the net.
		*/
		class SecurityManager
		{
			public:
				class KeyMissingException : public ibrcommon::Exception
				{
				public:
					KeyMissingException(std::string what = "Key for this operation is not available.") : ibrcommon::Exception(what)
					{};

					virtual ~KeyMissingException() throw() {};
				};

				class EncryptException : public ibrcommon::Exception
				{
				public:
					EncryptException(std::string what = "Encryption failed.") : ibrcommon::Exception(what)
					{};

					virtual ~EncryptException() throw() {};
				};

				class DecryptException : public ibrcommon::Exception
				{
				public:
					DecryptException(std::string what = "Decryption failed.") : ibrcommon::Exception(what)
					{};

					virtual ~DecryptException() throw() {};
				};

				class VerificationFailedException : public ibrcommon::Exception
				{
				public:
					VerificationFailedException(std::string what = "Verification failed.") : ibrcommon::Exception(what)
					{};

					virtual ~VerificationFailedException() throw() {};
				};

				/**
				 * Returns a singleton instance of this class.
				 * @return a reference to this class singleton
				 */
				static SecurityManager& getInstance();

				/**
				 * This method signs the bundle with the own private key. If no key is
				 * available a KeyMissingException is thrown.
				 * @param bundle A bundle to sign.
				 */
				void sign(dtn::data::Bundle &bundle) const throw (KeyMissingException);
				void auth(dtn::data::Bundle &bundle) const throw (KeyMissingException);

				/**
				 * This method should be called early as possible. It triggers a procedure to
				 * prepare the public key of some EID for later usage.
				 * @param eid The EID of the owner of the requested public key.
				 */
				void prefetchKey(const dtn::data::EID &eid);

				/**
				 * This method verifies the bundle and removes all auth or integrity block
				 * if they could validated.
				 * @param bundle The bundle to verify.
				 */
				void verify(dtn::data::Bundle &bundle) const throw (VerificationFailedException);
				void verifyBAB(dtn::data::Bundle &bundle) const throw (VerificationFailedException);
				void verifyPIB(dtn::data::Bundle &bundle) const throw (VerificationFailedException);

				/**
				 * This method do a fast verify with the bundle. It do not change anything in it.
				 * A missing key should not lead to an exception, because this method is
				 * called on each received and we need to support multihop without key
				 * knowledge too.
				 * @param bundle The bundle to verify.
				 */
				void fastverify(const dtn::data::Bundle &bundle) const throw (VerificationFailedException);

				/**
				 * This method decrypts encrypted payload of a bundle. It is necessary to
				 * remove all integrity or auth block before the payload can decrypted.
				 * @param bundle
				 */
				void decrypt(dtn::data::Bundle &bundle) const throw (DecryptException, KeyMissingException);

				/**
				 * This method encrypts the payload of a given bundle.
				 * If the bundle already contains integrity or auth block a EcryptException
				 * is thrown.
				 * @param bundle
				 */
				void encrypt(dtn::data::Bundle &bundle) const throw (EncryptException, KeyMissingException);

//				/**
//				If a Bundle is received, based on source and destination the
//				BundleAuthenticationBlocks and PayloadIntegrityBlocks are
//				checked and removed on validation. If this is the security
//				destination of the SecurityDestination PayloadConfidentialBlocks
//				and ExtensionSecurityBlocks these will be decrypted. If
//				everything is ok true will be returned, otherwise false.
//				@param bundle the bundle to be checked or decrypted
//				@return true if hashes and signs verify and decryption if
//				attempted succeeds, false otherwise
//				*/
//				bool incoming(dtn::data::Bundle&);
//
//				int outgoing_asymmetric(dtn::data::Bundle&, const dtn::data::EID&);
//
//				int outgoing_asymmetric(dtn::data::BundleID&);
//
//				/**
//				Applies PayloadIntegrityBlock,
//				PayloadConfidentialBlocks or ExtensionSecurityBlock to the
//				bundle based on policy before it will be send.
//				@param bundle the bundle to which BAB, PIB, PCB or ESB will be
//				applied to
//				@return 0 if all is ok, 1 if the bundle cannot be sent, because of
//				missing public keys
//				*/
//				int outgoing_asymmetric(dtn::data::Bundle&);
//
//				/**
//				 * Applies BundleAuthenticationBlocks to the Bundle
//				 */
//				int outgoing_p2p(const dtn::data::EID, dtn::data::Bundle&);

//				/**
//				 * Checks if all required information needed to send the bundle is
//				 * present. If keys are missing, they are requested from the network.
//				 */
//				bool isReadyToSend(const dtn::data::BundleID&);

//				/**
//				 * Checks if all required information needed to send the bundle is
//				 * present. If keys are missing, they are requested from the network.
//				 */
//				bool isReadyToSend(const dtn::data::Bundle&);

//				/**
//				Reads all security rules from the configuration
//				*/
//				void readRoutingTable();

//				/**
//				Deletes the lists and maps with the private and public keys. This is
//				needed if the keys are reread.
//				*/
//				void clearKeys();

//				/**
//				Loads a key from the Configuration (from the disk) into map, if it is
//				not there.
//				@param map a map of nodes and their keys
//				@param eid the eid of a node
//				@param bt the blocktype of the key
//				@return a pointer to the key or NULL if none exists
//				*/
//				static RSA * loadKey(std::map <dtn::data::EID, RSA* >&, const dtn::data::EID&, SecurityBlock::BLOCK_TYPES);
//
//				/**
//				 * If rsa is null the public key will be loaded from disk and
//				 * initializes rsa with this key. If there is no key rsa will be NULL.
//				 * @param rsa the key to be initialized if needed
//				 * @param bt the blocktype of the key
//				 * @return the RSA object if a key exists or NULL
//				 */
//				static RSA * loadKey_public(RSA ** rsa, SecurityBlock::BLOCK_TYPES bt);
//
//				/**
//				Loads a private key from the disk if it is not initialized and returns a
//				pointer to it, if it is not there NULL will be returned.
//				@param rsa a pointer to the pointer where the rsa object is stored
//				@param bt the blocktype of the key
//				@return a pointer to the key or NULL if none exists
//				*/
//				static RSA * loadKey(RSA **, dtn::security::SecurityBlock::BLOCK_TYPES);

//				/**
//				Reads a symmetric key from a file. If from the file cannot be read ""
//				will be returned.
//				@param file the file to read from
//				@return the key read from the file
//				*/
//				static std::string readSymmetricKey(const std::string&);

//				/**
//				Loads a symmetric key from map if it there. If not it reads the key from
//				disk and stores it into map.
//				@param map a map containing nodes and their keys
//				@param node the name of the node corresponding to the key
//				@param bt the block type of the key
//				@return the key or "" if none exists
//				*/
//				static std::string loadKey(std::map<dtn::data::EID, std::string>&, const dtn::data::EID&, SecurityBlock::BLOCK_TYPES);
//				static std::string loadKey(const dtn::data::EID&, SecurityBlock::BLOCK_TYPES);

			protected:
				/**
				need a list of nodes, their security blocks type and the key
				for private and public keys
				*/
				SecurityManager();

				virtual ~SecurityManager();

//				/**
//				 * Deletes a public foreign key for a specific node and block type from
//				 * this instance. It will remain on disk/flash. Nothing worse will
//				 * happen, if the key was not initialized.
//				 * @param node the name of the node
//				 * @param bt the block type of the key
//				 */
//				void deleteKey(const dtn::data::EID&, SecurityBlock::BLOCK_TYPES);

//				/**
//				Returns the associated RSA object with node and block type. When doing
//				this and you must be sure, that the key is present, you can check this
//				with isReadyToSend().
//				@param node the name of the owner of the public key
//				@param bt the block type for which the key shall be used for
//				@return a pointer to the RSA object, which represents the key. do not
//				delete it. If no key can be found NULL will be returned.
//				*/
//				RSA * getPublicKey(const dtn::data::EID&, dtn::security::SecurityBlock::BLOCK_TYPES);
//
//				/**
//				 * Returns our public key for the given block type.
//				 * @param bt the block type of the key
//				 * @return the RSA object for this block type
//				 */
//				RSA * getPublicKey(dtn::security::SecurityBlock::BLOCK_TYPES);
//
//				/**
//				Return the private key of the block type bt.
//				@param bt the block type of the key
//				@return a pointer to the private key for the block type
//				*/
//				RSA * getPrivateKey(dtn::security::SecurityBlock::BLOCK_TYPES);

//				/**
//				Returns the symmetric key for the given security block and node.
//				@return the symmetric key or "" if none exists
//				*/
//				std::string getSymmetricKey(const dtn::data::EID& node, SecurityBlock::BLOCK_TYPES bt);

				/**
				Check if we are the security_destination of some block
				@param bundle the complete bundle with all blocks
				@return true if there is a instance of type T and we are the security
				destination of it
				*/
				template<class T>
				bool contains_block_for_us_destination(const dtn::data::Bundle& bundle) const;

				/**
				Checks if there is e.g. a PIB, for which we have the public key and we
				might check it
				@param bundle the complete bundle with all blocks
				@param source the source we are looking for
				@return true if there is a block of type T and source matches as security source
				*/
				template<class T>
				bool contains_block_for_us_source(const dtn::data::Bundle& bundle, const dtn::data::EID& source) const;

				/**
				Checks if there is e.g. a PIB, for which we have the public key and we
				might check it
				@param bundle the complete bundle with all blocks
				@param source a list of sources we are looking for
				@return true if there is a block of type T and source matches as security source
				*/
				template<class T>
				bool contains_block_for_us_source(const dtn::data::Bundle& bundle, const std::list<dtn::data::EID>& eids) const;

				/**
				Tests if a PCB, which we are able to decrypt, verifies correctly.
				@param bundle the complete bundle with all blocks
				@param bab the first_pcb to be checked
				@return true if everything is ok
				*/
				bool check_pcb(dtn::data::Bundle& bundle, const dtn::security::PayloadConfidentialBlock& pcb);

				/**
				Tests if a PIB, which we are able to check, verifies correctly.
				@param bundle the complete bundle with all blocks
				@param bab the PIB to be checked
				@return true if everything is ok
				*/
				bool check_pib(dtn::data::Bundle& bundle, const dtn::security::PayloadIntegrityBlock& pib);

				/**
				Tests if a ESB, which we are able to decrypt, verifies correctly.
				@param bundle the complete bundle with all blocks
				@return true if everything is ok
				*/
				bool check_esb(dtn::data::Bundle& bundle);

				/**
				 * Adds all keys needed by a foreign node to successfully verify the
				 * given bundle to the bundle.
				 * @param bundle the bundle to which the keys are added which are
				 * necessary to verfiy it
				 */
				void addKeysForVerfication(dtn::data::Bundle&);

//				/**
//				 * Looks inside a bundle for stored keys and saves them locally or
//				 * updates old keys, if they are authenticated or similar. If a new key
//				 * arrived and will be stored newkeyReceived() will be called, which
//				 * looks after bundles with pending keys
//				 * @param bundle the bundle to be searched for keys
//				 */
//				void lookForKeys(const dtn::data::Bundle&);

//				/**
//				 * Looks after bundles with pending keys. If a bundle is ready to send,
//				 * an event will be generated, or something else.
//				 * @param node the name of the owner of the public key
//				 * @param bt the block type for which the key shall be used for
//				 */
//				void newKeyReceived(const dtn::data::EID&, dtn::security::SecurityBlock::BLOCK_TYPES);

				/**
				 * Searches in kbs if the key specified by node and bt is needed and
				 * returns the iterator to this block, which signals the requirement.
				 * kbs.end() is returned if the key from ksevt is not needed.
				 * @param node the owner of the public key
				 * @param bt the blocktype for the public key
				 * @param kbs a list in which all needed keys are specified
				 * @return an iterator to the element in kbs, which shows that the key
				 * is needed or kbs.end()
				 */
				std::list<dtn::security::KeyBlock>::const_iterator getMissingKey(const dtn::data::EID&, dtn::security::SecurityBlock::BLOCK_TYPES, const std::list<dtn::security::KeyBlock>&) const;

				/**
				Adds all Certificates needed to verify a pib to this bundle.
				@param bundle a bundle to which a pib was or will be added
				*/
				void addKeyBlocksForPIB(dtn::data::Bundle&);

				/**
				 * Returns the rule, which applies to the given EID
				 * @param eid the eid of the destination
				 * @return the rule, which shall be applied
				 */
				SecurityRule getRule(const dtn::data::EID&) const;

				std::list<dtn::security::KeyBlock> getListOfNeededSymmetricKeys(const dtn::data::Bundle&) const;

				std::list<dtn::security::KeyBlock> getListOfNeededKeys(const dtn::data::Bundle& bundle) const;

			private:
				bool _accept_only_bab;
				bool _accept_only_pib;

				// shared keys
				std::map<dtn::data::EID, std::string> _bab_node_key;

				// our private keys
				RSA * _pib_node_key_send;
				RSA * _pcb_node_key_recv;
				RSA * _esb_node_key_recv;

				// our public keys
				RSA * _pib_public_key;
				RSA * _pcb_public_key;
				RSA * _esb_public_key;

				// foreign public keys
				std::map<dtn::data::EID, RSA *> _pib_node_key_recv;
				std::map<dtn::data::EID, RSA *> _pcb_node_key_send;
				std::map<dtn::data::EID, RSA *> _esb_node_key_send;

				std::map<dtn::data::EID, SecurityRule> _security_rules_routing;

				/** stores each bundle with its needed keys for later sending */
				std::map<dtn::data::BundleID, std::list<dtn::security::KeyBlock> > _pending_keys;
				/**
				 * stores a bundle, with the neighbors to which it shall be send
				 * TODO not used atm
				 */
				std::map<dtn::data::BundleID, std::set<dtn::data::EID> > _bundle_with_neighbors;

				std::map<dtn::data::BundleID, dtn::data::EID> _bundle_with_origin;
		};

		template<class T>
		bool SecurityManager::contains_block_for_us_destination(const Bundle& bundle) const
		{
			const std::list<T const *> blocks = bundle.getBlocks<T>();
			for (typename std::list<T const *>::const_iterator it = blocks.begin(); it != blocks.end(); it++)
			{
				dtn::data::EID our_id = dtn::data::EID(dtn::daemon::Configuration::getInstance().getNodename());
				if ((**it).isSecurityDestination(bundle, our_id))
					return true;
			}
			return false;
		}

		template<class T>
		bool SecurityManager::contains_block_for_us_source(const Bundle& bundle, const dtn::data::EID& source) const
		{
			const std::list<T const *> blocks = bundle.getBlocks<T>();
			for (typename std::list<T const *>::const_iterator it = blocks.begin(); it != blocks.end(); it++)
			{
				dtn::data::EID ssource = (*it)->getSecuritySource();
				if (SecurityBlock::isSecuritySource(bundle, **it, source))
					return true;
			}
			return false;
		}

		template<class T>
		bool SecurityManager::contains_block_for_us_source(const Bundle& bundle, const std::list<dtn::data::EID>& eids) const
		{
			// for each eid call the method with one eid
			for (std::list<dtn::data::EID>::const_iterator it = eids.begin(); it != eids.end(); it++)
				if (contains_block_for_us_source<T>(bundle, *it))
					return true;
			return false;
		}
	}
}

#endif /* _SECURITY_MANAGER_H_ */
