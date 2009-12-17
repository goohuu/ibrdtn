/*
 * FileBundle.cpp
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#include "ibrdtn/api/FileBundle.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrcommon/data/BLOB.h"

namespace dtn
{
	namespace api
	{
		FileBundle::FileBundle(dtn::data::EID destination, string filename)
		 : Bundle(destination), _filename(filename)
		{
		}

//		FileBundle::FileBundle(dtn::data::EID destination, std::fstream &file)
//		 : Bundle(destination), _file(file)
//		{
//		}

		FileBundle::~FileBundle()
		{
		}

		void FileBundle::write(std::ostream &stream)
		{
			// create a reference out of the given file
			ibrcommon::BLOB::Reference ref = ibrcommon::FileBLOB::create(_filename);

			// create a memory based payload block.
			dtn::data::PayloadBlock *payload = new dtn::data::PayloadBlock(ref);

			// add the payload block to the bundle
			_b.addBlock(payload);

			// call Bundle::write()
			Bundle::write(stream);
		}
	}
}
