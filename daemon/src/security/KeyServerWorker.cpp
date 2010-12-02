#include "security/KeyServerWorker.h"
#include "Configuration.h"
#include "security/SecurityManager.h"
#include "security/SecurityKeyManager.h"
#include "core/BundleCore.h"
#include <ibrdtn/security/KeyBlock.h>
#include <ibrcommon/Logger.h>

#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif

namespace dtn
{
	namespace security
	{

		KeyServerWorker::KeyServerWorker()
		: _neighbors_age(0), _requestDistance(4)
		{
			AbstractWorker::initialize(KeyServerWorker::getSuffix(), true);
			bindEvent(KeyRequestEvent::getName_static());
		}

		KeyServerWorker::~KeyServerWorker()
		{
			unbindEvent(KeyRequestEvent::getName_static());
		}

		void KeyServerWorker::callbackBundleReceived(const Bundle &b)
		{
			dtn::core::BundleStorage& stor = core::BundleCore::getInstance().getStorage();
			dtn::data::BundleID id(b);
			dtn::data::Bundle b_non_const = stor.get(id);
			callbackBundleReceived_non_const(b_non_const);
		}

		void KeyServerWorker::callbackBundleReceived_non_const(Bundle &b)
		{
			// alle schlüsselanfragen durchgehen
			//  für jede beantwortbare schlüsselanfrage einen KeyBlock erstellen
			//  ursprünglich anfrage entfernen, wenn beantwortbar
			// antwort zurück schicken
			// falls noch anfragen übrig weiter schicken
			dtn::security::KeyBlock const * keyblockpt = NULL;
			try {
				keyblockpt = &(b.getBlock<dtn::security::KeyBlock>());
			}
			catch (...)
			{
				keyblockpt = NULL;
				return;
			}
			const dtn::security::KeyBlock& keyblock = *keyblockpt;

			dtn::core::BundleStorage& stor = dtn::core::BundleCore::getInstance().getStorage();

			// check if it was a response with a key
			// incoming bundle with key
			if (keyblock.getKey() != "")
			{
				// check if the key was requested and delete it from pending keys
				cleanRequestedKeys(keyblock);

				// maybe it was a response from a request to refresh the key and now the requester has the key
				std::list<BundleID>::const_iterator it = getPositionOfBundleWithKeyBlock(keyblock, stor, &(b._source));
				if (it != _requests_from_other_nodes.end())
				{
					dtn::data::BundleID bid = *it;
					_requests_from_other_nodes.remove(*it);
					stor.remove(bid);
				}

				stor.remove(b);
				return;
			}

			IBRCOMMON_LOGGER_ex(notice) << "Received new key request from " << b._source.getString() << " for node " << keyblock.getTarget().getString() << " with Securityblocktype " << keyblock.getSecurityBlockType() << " created" << IBRCOMMON_LOGGER_ENDL;

			// incoming bundle no key, neither from the bundle or from us
			// several cases, boolean keyblock.isSendable() and keyblock.isNeedingRefreshing()
			// 1. isSendable() && !isNeedingRefreshing() --> foreign node: distribute request or respond with key
			// 2. !isSendable() && !isNeedingRefreshing() --> foreign node: save bunddle, refresh counter at requester with new bundle, but test if we have the key present
			// 3. !isSendable() && isNeedingRefreshing() --> requesting node: refresh request with new bundle or respond with key to prove we have the key
			// 4. isSendable() && isNeedingRefreshing() --> foreign node: counter refreshed, reuse saved bundle

			// even more cases
			// what if the requesting node received the key in the time?
			// what if we can respond with the key?
			if (!keyblock.isNeedingRefreshing())
			{
				// if we have the key respond with the key and return
				RSA * rsa = KeyServerWorker::findPublicKey(keyblock.getTarget(), keyblock.getSecurityBlockType());
				if (rsa != NULL)
				{
					IBRCOMMON_LOGGER_ex(notice) << "Responding with key for " << keyblock.getTarget().getString() << " with Securityblocktype " << keyblock.getSecurityBlockType() << " to " << b._source.getString() << IBRCOMMON_LOGGER_ENDL;
					dtn::data::Bundle response;
					response._source = _eid;
					response._destination = b._source;
					dtn::security::KeyBlock& kb = response.push_back<dtn::security::KeyBlock>();
					kb.setTarget(keyblock.getTarget());
					kb.setSecurityBlockType(keyblock.getSecurityBlockType());
					kb.addKey(rsa);
					RSA_free(rsa);
					stor.remove(b);
					transmit(response);
					return;
				}

				if (keyblock.isSendable())
				{
					distributeRequest(b);
					return;
				}
				else
				{
					_requests_from_other_nodes.push_back(BundleID(b));
					Bundle bundle;
					bundle._source = _eid;
					bundle._destination = b._source;
					KeyBlock& kb = bundle.push_back<KeyBlock>();
					kb.setTarget(keyblock.getTarget());
					kb.setSecurityBlockType(keyblock.getSecurityBlockType());
					kb.setDistance(0);

#ifdef __DEVELOPMENT_ASSERTIONS__
					assert(!kb.isSendable());
#endif

					kb.setRefresh(true);

#ifdef __DEVELOPMENT_ASSERTIONS__
					assert(kb.isNeedingRefreshing());
#endif

					transmit(bundle);
					return;
				}
			}
			else
			{
				if (!keyblock.isSendable())
				{
					Bundle bundle;
					bundle._source = _eid;
					bundle._destination = b._source;
					KeyBlock& kb = bundle.push_back<KeyBlock>();
					kb.setTarget(keyblock.getTarget());
					kb.setSecurityBlockType(keyblock.getSecurityBlockType());
					kb.setDistance(_requestDistance);
					kb.setRefresh(true);

					RSA * rsa = KeyServerWorker::findPublicKey(keyblock.getTarget(), keyblock.getSecurityBlockType());
					if (rsa != NULL)
					{
						kb.addKey(rsa);
						RSA_free(rsa);
					}

#ifdef __DEVELOPMENT_ASSERTIONS__
					assert(kb.isSendable());
					assert(kb.isNeedingRefreshing());
#endif

					stor.remove(b);
					transmit(bundle);
					return;
				}
				else
				{
					// get bundle from the list and reset the counter
					list<BundleID>::const_iterator it = getPositionOfBundleWithKeyBlock(keyblock, stor);
					if (it != _requests_from_other_nodes.end())
					{
						Bundle distribute = stor.get(*it);
						_requests_from_other_nodes.remove(*it);
						KeyBlock& kb = distribute.getBlock<KeyBlock>();
						kb.setDistance(keyblock.getDistance());
						kb.setRefresh(false);
						stor.remove(b);
						distributeRequest(distribute);
					}
					return;
				}
			}
		}

		void KeyServerWorker::raiseEvent(const Event *evt)
		{
			// maybe it is a KeyRequestEvent
			if (evt->getName() == KeyRequestEvent::getName_static())
			{
				IBRCOMMON_LOGGER_ex(notice) << "Got new event: " << evt->getName() << IBRCOMMON_LOGGER_ENDL;
				KeyRequestEvent const * krevt = dynamic_cast<KeyRequestEvent const *>(evt);
				// look if the key has not already been requested
				if (isKeyRequested(krevt->getEID(), krevt->getBlockType()))
				{
					IBRCOMMON_LOGGER_ex(notice) << "Key already requested. Node: " << krevt->getEID().getString() << " with SecurityBlockType " << krevt->getBlockType() << IBRCOMMON_LOGGER_ENDL;
					return;
				}
				// look if the key is not already present
				if (SecurityKeyManager::getInstance().hasKey(krevt->getEID(), SecurityKeyManager::KEY_PUBLIC))
				{
					IBRCOMMON_LOGGER_ex(notice) << "Key already present. Node: " << krevt->getEID().getString() << " with SecurityBlockType " << krevt->getBlockType() << IBRCOMMON_LOGGER_ENDL;
					return;
				}

				requestKey(krevt->getEID(), krevt->getBlockType());
			}
		}

		void KeyServerWorker::distributeRequest(Bundle& request)
		{
			// to avoid amplification or excessively flooding, a unique identifier has
			// to be created and stored by each node for a amount of time. only
			// respond to not too all requests
			// if a bundle is identified by source and timestamp, changing the
			// destination should not be bad
			const time_t diff = 300;

			time_t ctime;
			std::time(&ctime);
			if (std::difftime(ctime, _neighbors_age) > diff)
			{
				_neighbors_age = ctime;
				_neighbors = core::BundleCore::getInstance().getNeighbors();
			}

			if (_neighbors.size() == 0)
				IBRCOMMON_LOGGER_ex(warning) << "No neighbors available for keyrequests!" << IBRCOMMON_LOGGER_ENDL;

			for (std::set<Node>::iterator it = _neighbors.begin(); it != _neighbors.end(); it++)
			{
				IBRCOMMON_LOGGER_ex(notice) << "Sending keyrequest to " << it->getURI() << IBRCOMMON_LOGGER_ENDL;
				request._destination = dtn::data::EID(it->getURI().append(KeyServerWorker::getSuffix()));
				transmit(request);
			}
		}

		void KeyServerWorker::requestKey(const dtn::data::EID& node, SecurityBlock::BLOCK_TYPES type)
		{
			IBRCOMMON_LOGGER_ex(notice) << "Requesting key for " << node.getString() << " with SecurityBlockType " << type << IBRCOMMON_LOGGER_ENDL;
			dtn::data::Bundle request;
			request._source = _eid;
			dtn::security::KeyBlock& kb = request.push_back<dtn::security::KeyBlock>();
			kb.setTarget(node);
			kb.setSecurityBlockType(type);
			kb.setDistance(_requestDistance);
			_requestedKeys.push_back(kb);
			distributeRequest(request);
		}

		RSA * KeyServerWorker::findPublicKey(const dtn::data::EID node, dtn::security::SecurityBlock::BLOCK_TYPES type)
		{
			RSA * rsa = NULL;
			SecurityKeyManager::SecurityKey key = SecurityKeyManager::getInstance().get(node.getNodeEID(), SecurityKeyManager::KEY_PUBLIC);

			if (key.data.size() > 0)
				SecurityManager::read_public_key(key.data, &rsa);

			return rsa;
		}

		bool KeyServerWorker::isKeyRequested(const dtn::data::EID& node, dtn::security::SecurityBlock::BLOCK_TYPES type) const
		{
			dtn::security::KeyBlock kb;
			kb.setTarget(node);
			kb.setSecurityBlockType(type);
			for (std::list <dtn::security::KeyBlock >::const_iterator it =_requestedKeys.begin(); it != _requestedKeys.end(); it++)
				if (kb == *it)
					return true;
			return false;
		}

		string KeyServerWorker::getSuffix()
		{
			std::string suffix("/keyserver");
			return suffix;
		}

		void KeyServerWorker::cleanRequestedKeys(const dtn::security::KeyBlock& keyblock)
		{
			//check if the key was requested and delete it from pending keys
			std::list<dtn::security::KeyBlock> deletable;
			for (list <dtn::security::KeyBlock >::iterator it = _requestedKeys.begin(); it != _requestedKeys.end(); it++)
			{
				if (keyblock == *it)
					deletable.push_back(*it);
			}
			for (list<dtn::security::KeyBlock >::iterator it = deletable.begin(); it != deletable.end(); it++)
				_requestedKeys.remove(*it);
		}

		list< BundleID >::const_iterator KeyServerWorker::getPositionOfBundleWithKeyBlock(const dtn::security::KeyBlock& keyblock, BundleStorage& stor, const dtn::data::EID * source) const
		{
			std::list<BundleID>::const_iterator it = _requests_from_other_nodes.begin();
			for (; it != _requests_from_other_nodes.end(); it++)
			{
				Bundle test = stor.get(*it);
				KeyBlock& kb = test.getBlock<KeyBlock>();
				if (kb.getTarget() == keyblock.getTarget() && kb.getSecurityBlockType() == keyblock.getSecurityBlockType() && (source == NULL || test._source == *source) )
					break;
			}
			return it;
		}

		KeyRequestEvent::~KeyRequestEvent()
		{
		}

		const std::string KeyRequestEvent::getName() const
		{
			return KeyRequestEvent::getName_static();
		}

		std::string KeyRequestEvent::toString() const
		{
			std::stringstream ss;
			ss << getName() << ": node of the key: " << _eid.getString() << ", type of the block: " << _blocktype;
			return ss.str();
		}

		const std::string KeyRequestEvent::getName_static()
		{
			return "KeyRequestEvent";
		}

		void KeyRequestEvent::raise(dtn::data::EID eid, dtn::security::SecurityBlock::BLOCK_TYPES bt)
		{
			IBRCOMMON_LOGGER_ex(notice) << "New key request for " << eid.getString() << " with Securityblocktype " << bt << " created" << IBRCOMMON_LOGGER_ENDL;
			raiseEvent(new KeyRequestEvent(eid.getNodeEID(), bt));
		}

		dtn::data::EID KeyRequestEvent::getEID() const
		{
			return _eid;
		}

		dtn::security::SecurityBlock::BLOCK_TYPES KeyRequestEvent::getBlockType() const
		{
			return _blocktype;
		}

		KeyRequestEvent::KeyRequestEvent(dtn::data::EID eid, dtn::security::SecurityBlock::BLOCK_TYPES bt)
		 : _eid(eid), _blocktype(bt)
		{
		}
	}
}
