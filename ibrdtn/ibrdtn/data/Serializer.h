// -*- C++ -*-
/* 
 * File:   Serializer.h
 * Author: morgenro
 *
 * Created on 30. Mai 2010, 15:31
 */

#ifndef _SERIALIZER_H
#define	_SERIALIZER_H

#include <iostream>
#include "ibrdtn/data/Dictionary.h"
#include "ibrdtn/data/PrimaryBlock.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/BundleFragment.h"

namespace dtn
{
	namespace data
	{
		class Bundle;
		class Block;
		class PrimaryBlock;
		class PayloadBlock;

		class Serializer
		{
		public:
			virtual ~Serializer() {};

			virtual Serializer &operator<<(const dtn::data::Bundle &obj) = 0;
			virtual Serializer &operator<<(const dtn::data::PrimaryBlock &obj) = 0;
			virtual Serializer &operator<<(const dtn::data::Block &obj) = 0;
			virtual Serializer &operator<<(const dtn::data::BundleFragment &obj)
			{
				(*this) << obj._bundle;
				return (*this);
			};

			virtual size_t getLength(const dtn::data::Bundle &obj) = 0;
			virtual size_t getLength(const dtn::data::PrimaryBlock &obj) const = 0;
			virtual size_t getLength(const dtn::data::Block &obj) const = 0;
		};

		class Deserializer
		{
		public:
			virtual ~Deserializer() {};

			virtual Deserializer &operator>>(dtn::data::Bundle &obj) = 0;
			virtual Deserializer &operator>>(dtn::data::PrimaryBlock &obj) = 0;
			virtual Deserializer &operator>>(dtn::data::Block &obj) = 0;
		};

		class Validator
		{
		public:
			class RejectedException : public dtn::SerializationFailedException
			{
			public:
				RejectedException(string what = "A validate method has the bundle rejected.") throw() : dtn::SerializationFailedException(what)
				{
				};
			};

			virtual void validate(const dtn::data::PrimaryBlock&) const throw (RejectedException) = 0;
			virtual void validate(const dtn::data::Block&, const size_t) const throw (RejectedException) = 0;
			virtual void validate(const dtn::data::Bundle&) const throw (RejectedException) = 0;
		};

		class AcceptValidator : public Validator
		{
		public:
			AcceptValidator();
			virtual ~AcceptValidator();

			virtual void validate(const dtn::data::PrimaryBlock&) const throw (RejectedException);
			virtual void validate(const dtn::data::Block&, const size_t) const throw (RejectedException);
			virtual void validate(const dtn::data::Bundle&) const throw (RejectedException);
		};

		class DefaultSerializer : public Serializer
		{
		public:
			/**
			 * Default serializer.
			 * @param stream Stream to write to
			 */
			DefaultSerializer(std::ostream &stream);

			/**
			 * Initialize the Serializer with a default dictionary. This will be used to write the
			 * right values in the EID reference part of blocks.
			 * @param stream Stream to write to
			 * @param d The default dictionary
			 */
			DefaultSerializer(std::ostream &stream, const Dictionary &d);

			/**
			 * Destructor
			 */
			virtual ~DefaultSerializer() {};

			virtual Serializer &operator<<(const dtn::data::Bundle &obj);
			virtual Serializer &operator<<(const dtn::data::PrimaryBlock &obj);
			virtual Serializer &operator<<(const dtn::data::Block &obj);
			virtual Serializer &operator<<(const dtn::data::BundleFragment &obj);

			virtual size_t getLength(const dtn::data::Bundle &obj);
			virtual size_t getLength(const dtn::data::PrimaryBlock &obj) const;
			virtual size_t getLength(const dtn::data::Block &obj) const;

		protected:
			Serializer &serialize(const dtn::data::PayloadBlock& obj, size_t clip_offset, size_t clip_length);
			void rebuildDictionary(const dtn::data::Bundle &obj);
			bool isCompressable(const dtn::data::Bundle &obj) const;
			std::ostream &_stream;

			Dictionary _dictionary;
			bool _compressable;
		};

		class DefaultDeserializer : public Deserializer
		{
		public:
			/**
			 * Default Deserializer
			 * @param stream Stream to read from
			 */
			DefaultDeserializer(std::istream &stream);

			/**
			 * Initialize the Deserializer
			 * The validator can check each block, header or bundle for validity.
			 * @param stream Stream to read from
			 * @param v Validator for the bundles and blocks
			 * @return
			 */
			DefaultDeserializer(std::istream &stream, Validator &v);

			/**
			 * Initialize the Deserializer with a default dictionary to reconstruct
			 * the right EID values of block if the primary header is not read by this
			 * Deserializer.
			 * @param stream Stream to read from
			 * @param d The default dictionary
			 */
			DefaultDeserializer(std::istream &stream, const Dictionary &d);

			/**
			 * Default destructor.
			 * @return
			 */
			virtual ~DefaultDeserializer() {};

			virtual Deserializer &operator>>(dtn::data::Bundle &obj);
			virtual Deserializer &operator>>(dtn::data::PrimaryBlock &obj);
			virtual Deserializer &operator>>(dtn::data::Block &obj);

		protected:
			std::istream &_stream;
			Validator &_validator;
			AcceptValidator _default_validator;

		private:
			Dictionary _dictionary;
			bool _compressed;
		};

		class SeparateSerializer : public DefaultSerializer
		{
		public:
			SeparateSerializer(std::ostream &stream);
			virtual ~SeparateSerializer();

			virtual Serializer &operator<<(const dtn::data::Block &obj);
			virtual size_t getLength(const dtn::data::Block &obj) const;
		};

		class SeparateDeserializer : public DefaultDeserializer
		{
		public:
			SeparateDeserializer(std::istream &stream, Bundle &b);
			virtual ~SeparateDeserializer();

			void readBlock();

			virtual Deserializer &operator>>(dtn::data::Block &obj);

		private:
			Bundle &_bundle;
		};
	}
}


#endif	/* _SERIALIZER_H */

