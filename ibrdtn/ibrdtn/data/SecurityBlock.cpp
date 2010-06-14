/*
 * SecurityBlock.cpp
 *
 *  Created on: 08.03.2010
 *      Author: morgenro
 */

#include "ibrdtn/data/SecurityBlock.h"
#include "ibrcommon/thread/MutexLock.h"

namespace dtn
{
	namespace data
	{
		SecurityBlock::SecurityBlock(SecurityBlock::BLOCK_TYPES type)
		 : Block(type)
		{

		}

		SecurityBlock::SecurityBlock(SecurityBlock::BLOCK_TYPES type, ibrcommon::BLOB::Reference)
		 : Block(type)
		{

		}

		SecurityBlock::~SecurityBlock()
		{
		}

		list<EID> SecurityBlock::getEIDList() const
		{
			std::list<EID> ret;

			if (_ciphersuite_flags & CONTINAS_SECURITY_SOURCE)
				ret.push_back(_security_source);

			if (_ciphersuite_flags & CONTINAS_SECURITY_DESTINATION)
				ret.push_back(_security_destination);

			return ret;
		}

		void SecurityBlock::read()
		{
//			ibrcommon::BLOB::Reference ref = getBLOB();
//			ibrcommon::MutexLock l(ref);
//
//			(*ref) >> _ciphersuite_id;
//			(*ref) >> _ciphersuite_flags;
//
//			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
//				(*ref) >> _correlator;
//
//			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
//				(*ref) >> _ciphersuite_params;
//
//			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
//				(*ref) >> _security_result;
		}

		void SecurityBlock::commit()
		{
//			ibrcommon::BLOB::Reference ref = getBLOB();
//			ibrcommon::MutexLock l(ref);
//
//			(*ref) << _ciphersuite_id << _ciphersuite_flags;
//
//			if (_ciphersuite_flags & CONTAINS_CORRELATOR)
//				(*ref) << _correlator;
//
//			if (_ciphersuite_flags & CONTAINS_CIPHERSUITE_PARAMS)
//				(*ref) << _ciphersuite_params;
//
//			if (_ciphersuite_flags & CONTAINS_SECURITY_RESULT)
//				(*ref) << _security_result;
		}
	}
}
