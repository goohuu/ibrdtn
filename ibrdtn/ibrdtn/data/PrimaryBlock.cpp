/*
 * PrimaryBlock.cpp
 *
 *  Created on: 26.05.2010
 *      Author: morgenro
 */

#include "ibrdtn/data/PrimaryBlock.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/utils/Utils.h"

namespace dtn
{
	namespace data
	{
		size_t PrimaryBlock::__sequencenumber = 0;

		PrimaryBlock::PrimaryBlock()
		 : _procflags(0), _timestamp(0), _sequencenumber(0), _lifetime(3600), _fragmentoffset(0), _appdatalength(0)
		{
			_timestamp = utils::Utils::get_current_dtn_time();
			_sequencenumber = __sequencenumber;
			__sequencenumber++;
		}

		PrimaryBlock::~PrimaryBlock()
		{
		}

		void PrimaryBlock::set(FLAGS flag, bool value)
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

		const bool PrimaryBlock::get(FLAGS flag) const
		{
			return (_procflags & flag);
		}

		bool PrimaryBlock::operator!=(const PrimaryBlock& other) const
		{
			return !((*this) == other);
		}

		bool PrimaryBlock::operator==(const PrimaryBlock& other) const
		{
			if (other._timestamp != _timestamp) return false;
			if (other._sequencenumber != _sequencenumber) return false;
			if (other._source != _source) return false;

			if (other._procflags & PrimaryBlock::FRAGMENT)
			{
				if (!get(PrimaryBlock::FRAGMENT)) return false;

				if (other._fragmentoffset != _fragmentoffset) return false;
				if (other._appdatalength != _appdatalength) return false;
			}

			return true;
		}

		bool PrimaryBlock::operator<(const PrimaryBlock& other) const
		{
			if (_source < other._source) return true;
			if (_timestamp < other._timestamp) return true;
			if (_sequencenumber < other._sequencenumber) return true;

			if (other.get(PrimaryBlock::FRAGMENT))
			{
				if (_fragmentoffset < other._fragmentoffset) return true;
			}

			return false;
		}

		bool PrimaryBlock::operator>(const PrimaryBlock& other) const
		{
			if (_source > other._source) return true;
			if (_timestamp > other._timestamp) return true;
			if (_sequencenumber > other._sequencenumber) return true;

			if (other.get(PrimaryBlock::FRAGMENT))
			{
				if (_fragmentoffset > other._fragmentoffset) return true;
			}

			return false;
		}

		bool PrimaryBlock::isExpired() const
		{
			if (utils::Utils::get_current_dtn_time() > (_lifetime + _timestamp)) return true;
			return false;
		}

		std::string PrimaryBlock::toString() const
		{
			stringstream ss;
			ss << "[" << _timestamp << "." << _sequencenumber << "] " << _source.getString() << " -> " << _destination.getString();
			return ss.str();
		}

		void PrimaryBlock::relabel()
		{
			_timestamp = utils::Utils::get_current_dtn_time();
			_sequencenumber = __sequencenumber;
			__sequencenumber++;
		}
	}
}
