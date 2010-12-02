#ifndef _KEY_SERVER_WORKER_H_
#define _KEY_SERVER_WORKER_H_

#include "core/AbstractWorker.h"
#include "core/EventReceiver.h"
#include "core/Event.h"
#include "core/BundleStorage.h"
#include <ibrdtn/security/SecurityBlock.h>
#include <ibrdtn/security/KeyBlock.h>
#include <openssl/rsa.h>
#include <ctime>

namespace dtn
{
	namespace security
	{
		/**
		 This programm will receive requests for public keys, with which it will
		 respond if found in its cache, or it will distribute the request to other
		 nodes. There can only be one request per bundle, because of a lot of
		 constness inside this programm. But multiple keys can be embedded into one
		 bundle.

		 Furthermore the SecurityManager can create KeyRequestEvents, when it wishes
		 to retrieve a key from the network, which is not stored locally.
		 */
		class KeyServerWorker : public dtn::core::AbstractWorker, public dtn::core::EventReceiver
		{
			public:
				KeyServerWorker();
				virtual ~KeyServerWorker();

				/**
				When a bundle is received. It will be checked, wether it carries a key
				(it will be dropped) or a key request (it will be distributed).
				This method retrieves the bundle from the storage, to get rid of the
				constness and passes it to the method without constness
				@param b the bundle from which the key was read, or with a key request,
				which will be distributed
				*/
				virtual void callbackBundleReceived(const Bundle &b);

				/**
				When a bundle is received. It will be checked, wether it carries a key
				(it will be dropped) or a key request (it will be distributed).
				@param b the bundle from which the key was read, or with a key request,
				which will be distributed
				*/
				void callbackBundleReceived_non_const(Bundle &b);

				/**
				Receive an Event. This should only be the KeyReceivedEvent or the
				KeyRequestEvent.
				@param evt the event to be examined
				*/
				virtual void raiseEvent(const dtn::core::Event *evt);

				/**
				Looks after a key or revocation certificate.
				TODO at the moment only the key is searched
				@return a pointer to the key, if one exists or NULL. Do not forget to
				delete the key with RSA_free()
				*/
				static RSA * findPublicKey(const dtn::data::EID node, dtn::security::SecurityBlock::BLOCK_TYPES type);

				/**
				Returns true, if a key has already been requested from the net.
				@param node the eid of the key owning node
				@param type the blocktype of the key
				@return true if the key has been requested, false otherwise
				*/
				bool isKeyRequested(const dtn::data::EID& node, dtn::security::SecurityBlock::BLOCK_TYPES type) const;

				/**
				Returns the application suffix, which will be appended to the nodename.
				@return the application suffix
				*/
				static std::string getSuffix();

			protected:
				/** keep all requested keys to avoid more than one request of a key */
				std::list<dtn::security::KeyBlock> _requestedKeys;

				/** this are our neighbors, which will sometimes be updated */
				std::set<dtn::core::Node> _neighbors;
				/** the time of the last update*/
				time_t _neighbors_age;
				/** saves the requests from other nodes for which the counter has to be refreshed */
				std::list<dtn::data::BundleID> _requests_from_other_nodes;

				unsigned int _requestDistance;

				/**
				 * Distributes a key request in the network.
				 * @param bundle a bundle containing the key request
				 */
				void distributeRequest(dtn::data::Bundle&);

				/**
				Sends a key request over the network.
				@param node the name of node
				@param type the type of the security block
				*/
				void requestKey(const dtn::data::EID& node, dtn::security::SecurityBlock::BLOCK_TYPES type);

				/**
				Removes all requests for the key this keyblock is carrying.
				@param keyblock a keyblock with a key
				*/
				void cleanRequestedKeys(const dtn::security::KeyBlock&);

				/**
				Seeks in _requests_from_other_nodes after a bundle with a keyblock equal to the one given.
				@param keyblock the keyblock of which the equal keyblock is searched
				@param stor the bundlestorage for retrieving the keyblocks
				@param source if given the bundles source must be the same
				@return a bundle with the given keyblock and which is from source, if given
				*/
				std::list<BundleID>::const_iterator getPositionOfBundleWithKeyBlock(const dtn::security::KeyBlock&, dtn::core::BundleStorage& , const dtn::data::EID * = NULL) const;
		};

		/**
		 * This event is created by the SecurityManager in order to tell the
		 * KeyServerWorker that it needs a key from the net.
		 */
		class KeyRequestEvent : public core::Event
		{
			public:
				/** does nothing */
				virtual ~KeyRequestEvent();
				/**
				 * Returns the name of this event
				 * @return the name of this event
				 */
				virtual const std::string getName() const;

				/**
				 * Returns the name of this event, the EID and the blocktype of the
				 * requested key.
				 * @return name of this event, EID and blocktype of the requested key
				 */
				virtual std::string toString() const;

				/**
				 * Returns the name of this event
				 * @return the name of this event
				 */
				static const std::string getName_static();

				/**
				 * Creates a new KeyRequestEvent
				 * @param eid the eid of the node, which owns the key
				 * @param bt the blocktype of the key
				 */
				static void raise(dtn::data::EID, dtn::security::SecurityBlock::BLOCK_TYPES);

				/**
				 * Returns the eid of the key owning node
				 * @return the eid of the key owning node
				 */
				dtn::data::EID getEID() const;

				/**
				 * Returns the blocktype of the key
				 * @return the blocktype of the key
				 */
				dtn::security::SecurityBlock::BLOCK_TYPES getBlockType() const;

				/**
				 * Creates a new KeyRequestEvent
				 * @param eid the eid of the node, which owns the key
				 * @param bt the blocktype of the key
				 */
				KeyRequestEvent(dtn::data::EID , dtn::security::SecurityBlock::BLOCK_TYPES );
			protected:
				/** the eid of the key owning node */
				dtn::data::EID _eid;
				/** the blocktype of the key */
				dtn::security::SecurityBlock::BLOCK_TYPES _blocktype;
		};
	}
}

#endif /* _KEY_SERVER_WORKER_H_ */
