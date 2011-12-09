/*
 * NodeHandshake.cpp
 *
 *  Created on: 09.12.2011
 *      Author: morgenro
 */

#include "routing/NodeHandshake.h"

namespace dtn
{
	namespace routing
	{
		NodeHandshake::NodeHandshake()
		 : _type(HANDSHAKE_INVALID)
		{
		}

		NodeHandshake::NodeHandshake(MESSAGE_TYPE type, size_t lifetime)
		 : _type(type), _lifetime(lifetime)
		{
		}

		NodeHandshake::~NodeHandshake()
		{
			for (std::list<NodeHandshakeItem*>::const_iterator iter = _items.begin(); iter != _items.end(); iter++)
			{
				delete (*iter);
			}

			for (std::map<size_t, std::stringstream* >::iterator iter = _raw_items.begin(); iter != _raw_items.end(); iter++)
			{
				delete (iter->second);
			}
		}

		void NodeHandshake::addRequest(const size_t identifier)
		{
			_requests.insert(identifier);
		}

		bool NodeHandshake::hasRequest(const size_t identifier) const
		{
			return (_requests.find(identifier) != _requests.end());
		}

		void NodeHandshake::addItem(NodeHandshakeItem *item)
		{
			_items.push_back(item);
		}

		bool NodeHandshake::hasItem(const size_t identifier) const
		{
			for (std::list<NodeHandshakeItem*>::const_iterator iter = _items.begin(); iter != _items.end(); iter++)
			{
				const NodeHandshakeItem &item = (**iter);
				if (item.getIdentifier() == identifier) return true;
			}

			return false;
		}

		NodeHandshakeItem* NodeHandshake::getItem(const size_t identifier) const
		{
			for (std::list<NodeHandshakeItem*>::const_iterator iter = _items.begin(); iter != _items.end(); iter++)
			{
				NodeHandshakeItem *item = (*iter);
				if (item->getIdentifier() == identifier) return item;
			}

			return NULL;
		}

		size_t NodeHandshake::getType() const
		{
			return _type;
		}

		size_t NodeHandshake::getLifetime() const
		{
			return _lifetime;
		}

		std::ostream& operator<<(std::ostream &stream, const NodeHandshake &hs)
		{
			// first the type as SDNV
			dtn::data::SDNV type(hs._type);
			stream << type;

			if (hs._type == NodeHandshake::HANDSHAKE_REQUEST)
			{
				// then the number of request items
				dtn::data::SDNV number_of_items(hs._requests.size());
				stream << number_of_items;

				for (std::set<size_t>::const_iterator iter = hs._requests.begin(); iter != hs._requests.end(); iter++)
				{
					dtn::data::SDNV req(*iter);
					stream << req;
				}
			}
			else if (hs._type == NodeHandshake::HANDSHAKE_RESPONSE)
			{
				// then the lifetime of this data
				dtn::data::SDNV lifetime(hs._lifetime);
				stream << lifetime;

				// then the number of request items
				dtn::data::SDNV number_of_items(hs._items.size());
				stream << number_of_items;

				for (std::list<NodeHandshakeItem*>::const_iterator iter = hs._items.begin(); iter != hs._items.end(); iter++)
				{
					const NodeHandshakeItem &item (**iter);

					// first the identifier of the item
					dtn::data::SDNV id(item.getIdentifier());
					stream << id;

					// then the length of the payload
					dtn::data::SDNV len(item.getLength());
					stream << len;

					item.serialize(stream);
				}
			}

			return stream;
		}

		std::istream& operator>>(std::istream &stream, NodeHandshake &hs)
		{
			// first the type as SDNV
			dtn::data::SDNV type;
			stream >> type;
			hs._type = type.getValue();

			if (hs._type == NodeHandshake::HANDSHAKE_REQUEST)
			{
				// then the number of request items
				dtn::data::SDNV number_of_items;
				stream >> number_of_items;

				for (size_t i = 0; i < number_of_items.getValue(); i++)
				{
					dtn::data::SDNV req;
					stream >> req;
					hs._requests.insert(req.getValue());
				}
			}
			else if (hs._type == NodeHandshake::HANDSHAKE_RESPONSE)
			{
				// then the lifetime of this data
				dtn::data::SDNV lifetime;
				stream >> lifetime;
				hs._lifetime = lifetime.getValue();

				// then the number of request items
				dtn::data::SDNV number_of_items;
				stream >> number_of_items;

				for (size_t i = 0; i < number_of_items.getValue(); i++)
				{
					// first the identifier of the item
					dtn::data::SDNV id;
					stream >> id;

					// then the length of the payload
					dtn::data::SDNV len;
					stream >> len;

					// add to raw map and create data container
					std::stringstream *data = new std::stringstream();
					hs._raw_items[id.getValue()] = data;

					// copy data to stringstream
					ibrcommon::BLOB::copy(*data, stream, len.getValue());
				}
			}

			return stream;
		}

		BloomFilterSummaryVector::BloomFilterSummaryVector(const SummaryVector &vector)
		 : _vector(vector)
		{
		}

		BloomFilterSummaryVector::BloomFilterSummaryVector()
		{
		}

		BloomFilterSummaryVector::~BloomFilterSummaryVector()
		{
		}

		size_t BloomFilterSummaryVector::getIdentifier() const
		{
			return identifier;
		}

		size_t BloomFilterSummaryVector::getLength() const
		{
			return _vector.getLength();
		}

		const SummaryVector& BloomFilterSummaryVector::getVector() const
		{
			return _vector;
		}

		std::ostream& BloomFilterSummaryVector::serialize(std::ostream &stream) const
		{
			stream << _vector;
			return stream;
		}

		std::istream& BloomFilterSummaryVector::deserialize(std::istream &stream)
		{
			stream >> _vector;
			return stream;
		}

		size_t BloomFilterSummaryVector::identifier = 1;

		BloomFilterPurgeVector::BloomFilterPurgeVector(const SummaryVector &vector)
		 : _vector(vector)
		{
		}

		BloomFilterPurgeVector::BloomFilterPurgeVector()
		{
		}

		BloomFilterPurgeVector::~BloomFilterPurgeVector()
		{
		}

		size_t BloomFilterPurgeVector::getIdentifier() const
		{
			return identifier;
		}

		size_t BloomFilterPurgeVector::getLength() const
		{
			return _vector.getLength();
		}

		const SummaryVector& BloomFilterPurgeVector::getVector() const
		{
			return _vector;
		}

		std::ostream& BloomFilterPurgeVector::serialize(std::ostream &stream) const
		{
			stream << _vector;
			return stream;
		}

		std::istream& BloomFilterPurgeVector::deserialize(std::istream &stream)
		{
			stream >> _vector;
			return stream;
		}

		size_t BloomFilterPurgeVector::identifier = 1;

	} /* namespace routing */
} /* namespace dtn */
