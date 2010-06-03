/*
 * Block.cpp
 *
 *  Created on: 04.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/Bundle.h"
#include <iostream>

using namespace std;

namespace dtn
{
	namespace data
	{
		Block::Block(char blocktype)
		 : _procflags(0), _blocktype(blocktype)
		{
		}

		Block::~Block()
		{
		}

		void Block::addEID(EID eid)
		{
			_eids.push_back(eid);

			// add proc flag if not set
			_procflags |= Block::BLOCK_CONTAINS_EIDS;
		}

		list<EID> Block::getEIDList() const
		{
			return _eids;
		}

		void Block::set(ProcFlags flag, bool value)
		{
			if (value)
			{
				_procflags |= flag;
			}
			else
			{
				_procflags &= ~(flag);
			}
		}

		const bool Block::get(ProcFlags flag) const
		{
			return (_procflags & flag);
		}
	}
}
