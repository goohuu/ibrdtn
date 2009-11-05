/*
 * BundleString.cpp
 *
 *  Created on: 11.09.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/BundleString.h"
#include "ibrdtn/data/SDNV.h"

namespace dtn
{
	namespace data
	{
		BundleString::BundleString(std::string value)
		 : std::string(value)
		{
		}

		BundleString::BundleString()
		{
		}

		BundleString::~BundleString()
		{
		}

		size_t BundleString::getLength() const
		{
			dtn::data::SDNV sdnv(length());
			return sdnv.getLength() + length();
		}

		std::ostream &operator<<(std::ostream &stream, const BundleString &bstring)
		{
			dtn::data::SDNV sdnv(bstring.length());
			stream << sdnv << (std::string&)bstring;
			return stream;
		}

		std::istream &operator>>(std::istream &stream, BundleString &bstring)
		{
			dtn::data::SDNV length;
			stream >> length;
			size_t data_len = length.getValue();

			char data[data_len+1];
			data[data_len] = '\0';
			stream.read(data, data_len);
			bstring.assign(data);

			return stream;
		}
	}
}
