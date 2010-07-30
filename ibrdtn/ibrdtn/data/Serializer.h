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

namespace dtn
{
    namespace data
    {
        class Bundle;
        class Block;
        class PrimaryBlock;

        class Serializer
        {
        public:
        	virtual ~Serializer() {};

            virtual Serializer &operator<<(const dtn::data::Bundle &obj) = 0;
            virtual Serializer &operator<<(const dtn::data::PrimaryBlock &obj) = 0;
            virtual Serializer &operator<<(const dtn::data::Block &obj) = 0;

            virtual size_t getLength(const dtn::data::Bundle &obj) const = 0;
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
        	~AcceptValidator();

            virtual void validate(const dtn::data::PrimaryBlock&) const throw (RejectedException);
            virtual void validate(const dtn::data::Block&, const size_t) const throw (RejectedException);
            virtual void validate(const dtn::data::Bundle&) const throw (RejectedException);
        };

        class DefaultSerializer : public Serializer
        {
        public:
            DefaultSerializer(std::ostream &stream);
            DefaultSerializer(std::ostream &stream, const Dictionary &d);
            virtual ~DefaultSerializer() {};

            virtual Serializer &operator<<(const dtn::data::Bundle &obj);
            virtual Serializer &operator<<(const dtn::data::PrimaryBlock &obj);
            virtual Serializer &operator<<(const dtn::data::Block &obj);

            virtual size_t getLength(const dtn::data::Bundle &obj) const;
            virtual size_t getLength(const dtn::data::PrimaryBlock &obj) const;
            virtual size_t getLength(const dtn::data::Block &obj) const;

        protected:
            void rebuildDictionary(const dtn::data::Bundle &obj);
            std::ostream &_stream;

        private:
            Dictionary _dictionary;
        };

        class DefaultDeserializer : public Deserializer
        {
        public:
        	DefaultDeserializer(std::istream &stream);
            DefaultDeserializer(std::istream &stream, Validator &v);
            DefaultDeserializer(std::istream &stream, const Dictionary &d);
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

