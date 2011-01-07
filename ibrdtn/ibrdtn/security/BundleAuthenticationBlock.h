#ifndef BUNDLE_AUTHENTICATION_BLOCK_H_
#define BUNDLE_AUTHENTICATION_BLOCK_H_

#include "ibrdtn/security/SecurityBlock.h"
#include "ibrdtn/data/ExtensionBlock.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/Bundle.h"

namespace dtn
{
	namespace security
	{
		/**
		Calculates the HMAC (Hashed Message Authentication Code) for P2P connections
		of security aware nodes. You can instantiate a factory of this class, which 
		will be given keys and EIDs of the corresponding nodes.\n
		You can use addMAC() to add BundleAuthenticationBlock pairs for each given 
		key to the bundle. If you have received a Bundle you can verify it by using 
		the method verify() and then remove all BundleAuthenticationBlocks by using
		removeAllBundleAuthenticationBlocks() from the bundle.
		*/
		class BundleAuthenticationBlock : public SecurityBlock
		{
			/**
			This class is allowed to call the parameterless contructor.
			*/
			friend class dtn::data::Bundle;
			public:
				class Factory : public dtn::data::ExtensionBlock::Factory
				{
				public:
					Factory() : dtn::data::ExtensionBlock::Factory(BundleAuthenticationBlock::BLOCK_TYPE) {};
					virtual ~Factory() {};
					virtual dtn::data::Block* create();
				};

				/** The block type of this class. */
				static const char BLOCK_TYPE = SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK;

				/**
				This creates the BundleAuthenticationBlock in the factory fashion for 
				signing bundles. The parameters we and partner are used to set security
				source and destination correctly so the verifieing node can pick the 
				right key.
				@param hkey key used for creating the HMAC. This class uses and manages
				a copy of it, so you are free to reuse or destroy it.
				@param size size of the key
				@param we the EID of this node
				@param partner the EID of the corresponding node of the key
				*/
				BundleAuthenticationBlock(const unsigned char * const hkey, const size_t size);

				/**
				This creates the BundleAuthenticationBlock in the factory fashion for 
				signing bundles. The parameters we and partners are used to set security
				source and destination correctly so the verifieing node can pick the 
				right key. As many keys as needed can be given to this constructor, but
				pay attention, that there is an equal number of partners.
				@param keys keys and their size used for creating the HMAC. Each key is copied, so you
				are free to reuse or destroy them.
				@param we the EID of this node
				@param partners the EIDs of the corresponding nodes of the keys
				*/
				BundleAuthenticationBlock(const std::list< std::pair<const unsigned char * const, const size_t> > keys);

				/**
				Deletes all keys, which were used for calculating the MACs
				*/
				virtual ~BundleAuthenticationBlock();

				/**
				Adds a key to the factory. To the key allways belongs a partner.
				@param hkey key used for creating the HMAC. This class uses and manages
				a copy of it, so you are free to reuse or destroy it.
				@param size size of the key
				@param partner the EID of the corresponding node of the key
				*/
				void addKey(const unsigned char * const hkey, const size_t size, const dtn::data::EID partner);

				/**
				Factory method to add a MAC to the bundle. The added BAB blocks will be 
				the first after the primary block and the last block. The last block 
				contains the MAC.
				@param bundle the bundle which will get a BAB pair
				*/
				void addMAC(dtn::data::Bundle &bundle) const;

				/**
				Tests if the bundles MAC is correct. There might be multiple BABs inside
				the bundle, which may be tested and the result will be 1 if one matches.
				None of these BABs will be removed.
				@param bundle the bundle to be checked
				@return -1 if an error occured, 0 if the signature does not match, 1 if
				the signature matches
				*/
				signed char verify(const dtn::data::Bundle &bundle) const;

				/**
				Removes all BundleAuthenticationBlocks from the bundle, which MACs 
				verify the bundle. It returns the number of BundleAuthenticationBlocks,
				which were not removed.
				@param bundle the bundle to be verified
				@return the number of non removed blocks
				*/
				int verifyAndRemoveMatchingBlocks(dtn::data::Bundle& bundle) const;

				/**
				Removes all BundleAuthenticationBlocks from a bundle
				@param bundle the bundle, which shall be cleaned from babs
				*/
				static void removeAllBundleAuthenticationBlocks(dtn::data::Bundle& bundle);

			protected:
				/**
				Creates an empty BundleAuthenticationBlock. This BAB is meant to be 
				inserted into a bundle, by a factory. Because the instantiation will be
				done by the bundle instance for memory management, this method will be
				called be the bundle. The ciphersuite id is set to BAB_HMAC.
				*/
				BundleAuthenticationBlock();

				/**
				Creates the MAC of a given bundle using the BAB_HMAC algorithm. If a 
				correlator is given the MAC is created for the primary block and the 
				part of the bundle between the two BABs with the correlator.
				@param bundle bundle of which the MAC shall be calculated
				@param key the key to be used for creating the MAC
				@param key_size the size of the key
				@param with_correlator tells if a correlator shall be used
				@param correlator the correlator which shall be used
				@return a string containing the MAC
				*/
				std::string calcMAC(const dtn::data::Bundle& bundle, const unsigned char * const key, const size_t key_size, const bool with_correlator = false, const u_int64_t correlator = 0) const;

				/**
				Tries to verify the bundle using the given key. If a BAB-pair is found,
				which contains a valid hash corresponding to the key, the first value of
				the returned pair is true and the second contains the correlator.
				otherwise the first value is false and the second undefined.
				@param bundle bundle which shall be verified
				@param key the key for testing
				@return first is true if the key matched and second is the correlator of
				the matching pair. otherwise the first is false, if there was no
				matching
				*/
				std::pair<bool,u_int64_t> verify(const dtn::data::Bundle &bundle, std::pair<const unsigned char * const, const size_t> key, const dtn::data::EID& partner) const;

				/**
				Returns the size of the security result field. This is used for strict 
				canonicalisation, where the block itself is included to the canonical 
				form, but the security result is excluded or unknown.
				*/
				virtual size_t getSecurityResultSize() const;
			private:
				/**
				A list of keys and their size. The keys are copied at the destructor and
				will be deleted by the destructor.
				*/
				std::list<std::pair<const unsigned char * const, const size_t> > _keys;
				
				/**
				A list of EIDs which belong to the keys.
				TODO It would have been better to bundle this with the _keys member
				*/
				std::list<dtn::data::EID> _partners;
		};

		/**
		 * This creates a static block factory
		 */
		static BundleAuthenticationBlock::Factory __BundleAuthenticationBlockFactory__;
	}
}

#endif /* BUNDLE_AUTHENTICATION_BLOCK_H_ */
