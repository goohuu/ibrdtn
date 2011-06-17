/*
 * PlainSerializer.h
 *
 *  Created on: 16.06.2011
 *      Author: morgenro
 */

#ifndef PLAINSERIALIZER_H_
#define PLAINSERIALIZER_H_

#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/PrimaryBlock.h>
#include <ibrdtn/data/Block.h>

namespace dtn
{
	namespace api
	{
		class PlainSerializer : public dtn::data::Serializer
		{
		public:
			PlainSerializer(std::ostream &stream);
			virtual ~PlainSerializer();

			dtn::data::Serializer &operator<<(const dtn::data::Bundle &obj);
			dtn::data::Serializer &operator<<(const dtn::data::PrimaryBlock &obj);
			dtn::data::Serializer &operator<<(const dtn::data::Block &obj);

			size_t getLength(const dtn::data::Bundle &obj);
			size_t getLength(const dtn::data::PrimaryBlock &obj) const;
			size_t getLength(const dtn::data::Block &obj) const;

		private:
			std::ostream &_stream;
		};

		class PlainDeserializer : public dtn::data::Deserializer
		{
		public:
			PlainDeserializer(std::istream &stream);
			virtual ~PlainDeserializer();

			dtn::data::Deserializer &operator>>(dtn::data::Bundle &obj);
			dtn::data::Deserializer &operator>>(dtn::data::PrimaryBlock &obj);
			dtn::data::Deserializer &operator>>(dtn::data::Block &obj);

		private:
			std::istream &_stream;
		};
	}
}

#endif /* PLAINSERIALIZER_H_ */
