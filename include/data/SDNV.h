/*
 *    Copyright 2005-2006 Intel Corporation
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *    THIS FILE BASES ON DTN_2.4.0/SERVLIB/BUNDLING/SDNV.H
 */

#include <sys/types.h>
#include <iostream>
#include <stdio.h>

#ifndef _SDNV_H_
#define _SDNV_H_

/**
 * Class to handle parsing and formatting of self describing numeric
 * values (SDNVs).
 *
 * The basic idea is to enable a compact byte representation of
 * numeric values that may widely vary in size. This encoding is based
 * on the ASN.1 specification for encoding Object Identifier Arcs.
 *
 * Conceptually, the integer value to be encoded is split into 7-bit
 * segments. These are encoded into the output byte stream, such that
 * the high order bit in each byte is set to one for all bytes except
 * the last one.
 *
 * Note that this implementation only handles values up to 64-bits in
 * length (since the conversion is between a u_int64_t and the encoded
 * byte sequence).
 */

namespace dtn
{
namespace data
{
	class SDNV {
	public:
	    /**
	     * The maximum length for this SDNV implementation is 10 bytes,
	     * since the maximum value is 64 bits wide.
	     */
	    static const size_t MAX_LENGTH = 10;

	    // INLINE-FUNCTIONS
	    inline static size_t encoding_len(u_int16_t val){ return encoding_len((u_int64_t)val); }
	    inline static size_t encoding_len(u_int32_t val){ return encoding_len((u_int64_t)val); }
	    inline static int encode(u_int16_t val, char* bp, size_t len){ return encode((u_int64_t)val, (u_char*)bp, len); }
	    inline static int encode(u_int32_t val, char* bp, size_t len){ return encode((u_int64_t)val, (u_char*)bp, len); }
	    inline static int encode(u_int64_t val, char* bp, size_t len){ return encode(val, (u_char*)bp, len); }
	    inline static int encode(float val, char* bp, size_t len){ return encode(val, (u_char*)bp, len); }
	    inline static int decode(char* bp, size_t len, u_int64_t* val){ return decode((u_char*)bp, len, val); }
	    inline static int decode(char* bp, size_t len, u_int32_t* val){ return decode((u_char*)bp, len, val); }

	    static size_t encoding_len(u_int64_t val);
	    static size_t encoding_len(float val_f);
	    static int encode(u_int64_t val, u_char* bp, size_t len);
	    static int encode(float val_f, u_char* bp, size_t len);
	    static int decode(const u_char* bp, size_t len, float* val_f);
	    static int decode(const u_char* bp, size_t len, u_int64_t* val);
	    static int decode(const u_char* bp, size_t len, u_int32_t* val);
	    static int decode(const u_char* bp, size_t len, u_int16_t* val);
	    static size_t len(const u_char* bp);
	};
}
}

#endif /* _SDNV_H_ */
