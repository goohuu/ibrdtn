/*
 * FileBundle.cpp
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/FileBundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrcommon/data/BLOB.h"
#include <cassert>

namespace dtn
{
	namespace api
	{
		FileBundle::FileBundle(dtn::data::EID destination, string filename)
		 : Bundle(destination), _filename(filename)
		{
			// create a reference out of the given file
			ibrcommon::BLOB::Reference ref = ibrcommon::FileBLOB::create(_filename);

			// create a memory based payload block.
			dtn::data::PayloadBlock &payload = _b.push_back(ref);
		}

		FileBundle::~FileBundle()
		{
		}
	}
}
