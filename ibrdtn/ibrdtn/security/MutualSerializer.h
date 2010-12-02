#ifndef __MUTUAL_SERIALIZER_H__
#define __MUTUAL_SERIALIZER_H__

#include "ibrdtn/data/Serializer.h"
#include "ibrdtn/data/SDNV.h"
#include <sys/types.h>

namespace dtn
{
	namespace security
	{
		class SecurityBlock;

		/**
		Serializes a bundle in mutable canonical form into a given stream. In 
		mutable canonical form all SDNVs are unpacked to 8 byte values and all EID 
		references are filled with the EID they point to. Every number is written in
		network byte order. Only payload related blocks are serialized. These are 
		the payload block, the PayloadIntegrityBlock and the 
		PayloadConfidentialBlock.
		*/
		class MutualSerializer : public dtn::data::Serializer
		{
			public:
				/**
				The size in bytes of a SDNV in mutable form in the stream
				*/
				static const size_t sdnv_size = 8;
				
				/**
				Creates a MutualSerializer which will stream into stream
				@param stream the stream in which the mutable canonical form will be 
				written into
				*/
				MutualSerializer(std::ostream& stream);
				
				/** does nothing */
				virtual ~MutualSerializer();
				
				/**
				Serializes the given bundle into the stream, which was given at the 
				constructor in to mutable canonical form.
				@param obj the bundle which shall be serialized into the mutable 
				canonical form.
				@param ignore a pointer to the block, which security result shall be ignored
				@return a reference to this instance
				*/
				virtual MutualSerializer& serialize_mutable(const dtn::data::Bundle &obj, const dtn::security::SecurityBlock * ignore);
				
				/**
				Serializes the given bundle into the stream, which was given at the 
				constructor in to mutable canonical form. The security result of no 
				block will be ignored.
				@param obj
				@return a reference to this instance
				*/
				virtual MutualSerializer &operator<<(const dtn::data::Bundle &obj);
				
				/**
				Serializes the primary block in mutable canonical form. The usual rules 
				for mutable canonicalisation (network byte order, unpacked SDNV, full 
				EIDs instead of references) apply here and some bits of the flags are 
				set to zero during this operation.
				@return a reference to this instance
				*/
				virtual MutualSerializer &operator<<(const dtn::data::PrimaryBlock &obj);
				
				/**
				Serializes the block in mutable canonical form. The usual rules 
				for mutable canonicalisation (network byte order, unpacked SDNV, full 
				EIDs instead of references) apply here and some bits of the flags are 
				set to zero during this operation.
				@return a reference to this instance
				*/
				virtual MutualSerializer &operator<<(const dtn::data::Block &obj);
				
				/**
				Not implemented. This is only required by the interface.
				*/
				virtual size_t getLength(const dtn::data::Bundle &obj);
				
				/**
				Returns the length of the primary block in mutable canonical form.
				@param obj the primary block, of which the length shall be calculated
				@return the length of the primary block
				*/
				virtual size_t getLength(const dtn::data::PrimaryBlock &obj) const;
				
				/**
				Returns the length of the block in mutable canonical form.
				@param obj the block, of which the length shall be calculated
				@return the length of the block
				*/
				virtual size_t getLength(const dtn::data::Block &obj) const;

				/**
				Writes a u_int32_t into stream in network byte order.
				@param stream the stream in which shall be written
				@param value the value to be written in network byte order
				@return the stream in which shall be written
				*/
				static std::ostream& write_mutable(std::ostream& stream, const u_int32_t value);
				
				/**
				Writes an EID to a stream in mutable form.
				@param stream the stream in which shall be written
				@param value the EID which shall be written
				@return the stream in which shall be written
				*/
				static std::ostream& write_mutable(std::ostream& stream, const dtn::data::EID& value);
				
				/**
				Writes a SDNV to a stream in mutable form.
				@param stream the stream in which shall be written
				@param value the SDNV which shall be written
				@return the stream in which shall be written
				*/
				static std::ostream& write_mutable(std::ostream& stream, const dtn::data::SDNV& value);

			private:
				/** the stream which is given at the constructor and in which the 
				serialized bundle will be written into */
				std::ostream& _stream;
		};
	}
}

#endif
