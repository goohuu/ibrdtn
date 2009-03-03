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
*    THIS FILE BASES ON DTN_2.4.0/SERVLIB/BUNDLING/SDNV.CC
*/

using namespace std;

#include "data/SDNV.h"
#include "data/Exceptions.h"
#include <cstdlib>
#include <cstring>

namespace dtn
{
namespace data
{
	/**
	* Convert the given float into an SDNV.
	*
	* @return The number of bytes used, or -1 on ERROR(SDNV).
	*/
	int SDNV::encode(float val_f, u_char* bp, size_t len){
	  // Convert the float to an int and call the encoding_len(int) function
	  u_int32_t val_i;
	  memcpy(&val_i, &val_f, sizeof(u_int32_t));

	  return encode((u_int64_t)val_i, bp, len);
	}

	/**
	* Convert the given 64-bit integer into an SDNV.
	*
	* @return The number of bytes used, or -1 on ERROR(SDNV).
	*/
	int SDNV::encode(u_int64_t val, u_char* bp, size_t len){
	  u_char* start = bp;

	  //Figure out how many bytes we need for the encoding.
	  size_t val_len = 0;
	  u_int64_t tmp = val;

	  do {
	    tmp = tmp >> 7;
	    val_len++;
	  } while (tmp != 0);

	  if (!(val_len > 0)) throw exceptions::SDNVDecodeFailed("ERROR(SDNV): !(val_len > 0)");
	  if (!(val_len <= MAX_LENGTH)) throw exceptions::SDNVDecodeFailed("ERROR(SDNV): !(val_len <= MAX_LENGTH)");
	  // Make sure we have enough buffer space.
	  if (len < val_len) {
	    return -1;
	  }

	  // Now advance bp to the last byte and fill it in backwards with the value bytes.
	  bp += val_len;
	  u_char high_bit = 0; // for the last octet
	  do {
	    --bp;
	    *bp = (u_char)(high_bit | (val & 0x7f));
	    high_bit = (1 << 7); // for all but the last octet
	    val = val >> 7;
	  } while (val != 0);

	  if (!(bp == start)) throw exceptions::SDNVDecodeFailed("ERROR(SDNV): !(bp == start)");

	  return val_len;
	}


	/**
	* Return the number of bytes needed to encode the given value.
	*/
	size_t SDNV::encoding_len(u_int64_t val){
	  u_char buf[16];
	  int ret = encode(val, buf, sizeof(buf));
	  if (!(ret != -1 && ret != 0)) throw exceptions::SDNVDecodeFailed("ERROR(SDNV): !(ret != -1 && ret != 0)");
	  return ret;
	}

	/**
	* Return the number of bytes needed to encode the given float value.
	*/
	size_t SDNV::encoding_len(float val_f){
	  // Convert the float to an int and call the encoding_len(int) function
	  u_int32_t val_i;
	  memcpy(&val_i, &val_f, sizeof(u_int32_t));

	  return encoding_len(val_i);
	}

	/**
	* Convert an SDNV pointed to by bp into a unsigned 64-bit
	* integer.
	*
	* @return The number of bytes of bp consumed, or -1 on ERROR(SDNV).
	*/
	int SDNV::decode(const u_char* bp, size_t len, u_int64_t* val){
	  const u_char* start = bp;

	  if (!val) {
	    throw exceptions::SDNVDecodeFailed();
	  }

	  // Zero out the existing value, then shift in the bytes of the
	  // encoding one by one until we hit a byte that has a zero high-order bit.
	  size_t val_len = 0;
	  *val = 0;
	  do {
	    if (len == 0)
	      return -1; // buffer too short

	    *val = (*val << 7) | (*bp & 0x7f);
	    ++val_len;

	    if ((*bp & (1 << 7)) == 0)
	      break; // all done;

	    ++bp;
	    --len;
	  } while (1);


	  // Since the spec allows for infinite length values but this
	  // implementation only handles up to 64 bits, check for overflow.
	  // Note that the only supportable 10 byte SDNV must store exactly
	  // one bit in the first byte of the encoding (i.e. the 64'th bit
	  // of the original value).
	  // This is OK because a spec update says that behavior
	  // is undefined for values > 64 bits.
	  if ((val_len > MAX_LENGTH) || ((val_len == MAX_LENGTH) && (*start != 0x81))){
	    throw exceptions::SDNVDecodeFailed("ERROR(SDNV): overflow value in sdnv");
	  }

	  return val_len;
	}

	/**
	* Convert an SDNV pointed to by bp into a unsigned 32-bit
	* integer. Checks for overflow in the SDNV.
	*
	* @return The number of bytes of bp consumed, or -1 on ERROR(SDNV).
	*/
	int SDNV::decode(const u_char* bp, size_t len, u_int32_t* val){
	  u_int64_t lval;
	  int ret = decode(bp, len, &lval);

	  if (lval > 0xffffffffLL) {
	    throw exceptions::SDNVDecodeFailed();
	  }

	  *val = (u_int32_t)lval;

	  return ret;
	}

	/**
	* Convert an SDNV pointed to by bp into a unsigned 16-bit
	* integer. Checks for overflow in the SDNV.
	*
	* @return The number of bytes of bp consumed, or -1 on ERROR(SDNV).
	*/
	int SDNV::decode(const u_char* bp, size_t len, u_int16_t* val){
	  u_int64_t lval;
	  int ret = decode(bp, len, &lval);

	  if (lval > 0xffffffffLL) {
	    throw exceptions::SDNVDecodeFailed();
	  }

	  *val = (u_int16_t)lval;

	  return ret;
	}

	/**
	* Convert an SDNV pointed to by bp into a float
	* Checks for overflow in the SDNV.
	*
	* @return The number of bytes of bp consumed, or -1 on ERROR(SDNV).
	*/
	int SDNV::decode(const u_char* bp, size_t len, float* val){
	  u_int64_t lval;
	  int ret = decode(bp, len, &lval);

	  if (lval > 0xffffffffLL) {
	    throw exceptions::SDNVDecodeFailed();
	  }

	  float fval;
	  u_int32_t ival = (u_int32_t)lval;
	  memcpy(&fval, &ival, sizeof(u_int32_t));

	  *val = fval;

	  return ret;
	}

	/**
	* Return the number of bytes which comprise the given value.
	* Assumes that bp points to a valid encoded SDNV.
	*/
	size_t SDNV::len(const u_char* bp){
	  size_t val_len = 1;

	  for ( ; *bp++ & 0x80; ++val_len )
	    ;
	  return val_len;
	}
}
}
