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
#include "PrimaryBlock.h"

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
            virtual Serializer &operator<<(const dtn::data::Bundle &obj) = 0;
            virtual Serializer &operator<<(const dtn::data::PrimaryBlock &obj) = 0;
            virtual Serializer &operator<<(const dtn::data::Block &obj) = 0;

            virtual const size_t getLength(const dtn::data::Bundle &obj) const = 0;
            virtual const size_t getLength(const dtn::data::PrimaryBlock &obj) const = 0;
            virtual const size_t getLength(const dtn::data::Block &obj) const = 0;
        };

        class Deserializer
        {
        public:
            virtual Deserializer &operator>>(dtn::data::Bundle &obj) = 0;
            virtual Deserializer &operator>>(dtn::data::PrimaryBlock &obj) = 0;
            virtual Deserializer &operator>>(dtn::data::Block &obj) = 0;
        };

        class DefaultSerializer : public Serializer
        {
        public:
            DefaultSerializer(std::ostream &stream);
            DefaultSerializer(std::ostream &stream, const Dictionary &d);

            virtual Serializer &operator<<(const dtn::data::Bundle &obj);
            virtual Serializer &operator<<(const dtn::data::PrimaryBlock &obj);
            virtual Serializer &operator<<(const dtn::data::Block &obj);

            virtual const size_t getLength(const dtn::data::Bundle &obj) const;
            virtual const size_t getLength(const dtn::data::PrimaryBlock &obj) const;
            virtual const size_t getLength(const dtn::data::Block &obj) const;

        protected:
            void rebuildDictionary(const dtn::data::Bundle &obj);

        private:
            std::ostream &_stream;
            Dictionary _dictionary;
        };

        class DefaultDeserializer : public Deserializer
        {
        public:
            DefaultDeserializer(std::istream &stream);
            DefaultDeserializer(std::istream &stream, const Dictionary &d);
            
            virtual Deserializer &operator>>(dtn::data::Bundle &obj);
            virtual Deserializer &operator>>(dtn::data::PrimaryBlock &obj);
            virtual Deserializer &operator>>(dtn::data::Block &obj);

        private:
            std::istream &_stream;
            Dictionary _dictionary;
        };
    }
}


#endif	/* _SERIALIZER_H */

