/*
 * BundleStreamTestsuite.cpp
 *
 *  Created on: 25.06.2009
 *      Author: morgenro
 */

#include "testsuite/BundleStreamTestSuite.h"

#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/BLOBManager.h"
#include "ibrdtn/data/BLOBReference.h"
#include <fstream>
#include <stdlib.h>
#include <list>

using namespace dtn::data;
using namespace dtn::blob;
using namespace std;

namespace dtn
{
	namespace testsuite
	{
		BundleStreamTestSuite::BundleStreamTestSuite()
		 : _blobmanager(BLOBManager::_instance)
		{
		}

		BundleStreamTestSuite::~BundleStreamTestSuite()
		{
		}

		bool BundleStreamTestSuite::runAllTests()
		{
			bool ret = true;
			cout << "BundleStreamTestSuite... ";

			if ( !commonTest() )
			{
				cout << endl << "commonTest failed" << endl;
				ret = false;
			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool BundleStreamTestSuite::commonTest()
		{
			// create a bundle
			Bundle b;
			PayloadBlock *block = createTestBlock();
			stringstream container;

			// add block to bundle
			b.addBlock( block );

			// set attributes of the bundle
			b._timestamp = 4;
			b._sequencenumber = 6;
			b._source = EID("dtn://hq.bs.ibr.dtn/info");
			b._destination = EID("dtn://tram5.bs.ibr.dtn/info");

			// write the bundle to a container
			container << b;

			// read the bundle from a container
			container >> b;

			if (b._destination != EID("dtn://tram5.bs.ibr.dtn/info"))
			{
				cout << "Destination: " << b._destination.getString() << endl;
				return false;
			}

			if (b._source != EID("dtn://hq.bs.ibr.dtn/info"))
			{
				cout << "Source: " << b._destination.getString() << endl;
				return false;
			}

			const list<Block*> blocks = b.getBlocks();
			list<Block*>::const_iterator iter = blocks.begin();

			if (blocks.size() != 1)
			{
				cout << "wrong count of blocks " << blocks.size() << endl;
				return false;
			}

			while (iter != blocks.end())
			{
				CustodySignalBlock *signal = dynamic_cast<CustodySignalBlock*>(*iter);
				if (signal != NULL)
				{
					signal->read();
					cout << "custody signal received: ";
					cout << signal->_source.getString() << endl;
				}

				StatusReportBlock *report = dynamic_cast<StatusReportBlock*>(*iter);
				if (report != NULL)
				{
					report->read();
					cout << "status report received: ";
					cout << report->_source.getString() << endl;
				}

				iter++;
			}

			return true;
		}

		PayloadBlock* BundleStreamTestSuite::createTestBlock()
		{
			// create a blob object
			BLOBReference blobref = _blobmanager.create();

			// create a payload block, managed in memory
			CustodySignalBlock *p = new CustodySignalBlock( blobref );

			p->_source = EID("dtn://ibr.dtn");
			p->commit();

			return p;
		}
	}
}
