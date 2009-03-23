/*
 * StatusReportTestSuite.h
 *
 *  Created on: 20.03.2009
 *      Author: morgenro
 */

#include "testsuite/StatusReportTestSuite.h"
#include "testsuite/SelfTestSuite.h"
#include "testsuite/TestUtils.h"
#include "data/Bundle.h"
#include "data/BundleFactory.h"
#include "data/PayloadBlockFactory.h"
#include "data/StatusReportBlock.h"
#include <iostream>
#include "utils/Utils.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	StatusReportTestSuite::StatusReportTestSuite()
	{
	}

	StatusReportTestSuite::~StatusReportTestSuite()
	{
	}

	bool StatusReportTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "StatusReportTestSuite... ";

		if ( !createTest() )
		{
			cout << endl << "createTest failed" << endl;
			ret = false;
		}

		if ( !parseTest() )
		{
			cout << endl << "parseTest failed" << endl;
			ret = false;
		}

		if ( !fragmentTest() )
		{
			cout << endl << "fragmentTest failed" << endl;
			ret = false;
		}

		if ( !matchTest() )
		{
			cout << endl << "signalTest failed" << endl;
			ret = false;
		}

		if (ret) cout << "\t\tpassed" << endl;

		return ret;
	}

	bool StatusReportTestSuite::fragmentTest()
	{
		bool ret = true;

		Bundle *out = TestUtils::createTestBundle(1000);
		list<Bundle> bundles = BundleFactory::split(*out, 200);

		Bundle &bundle = bundles.back();
		BundleFactory &fac = BundleFactory::getInstance();
		Bundle *cbundle = fac.newBundle();
		StatusReportBlock *block = PayloadBlockFactory::newStatusReportBlock();
		cbundle->appendBlock( block );

		block->setMatch( bundle );

		if (!bundle.getPrimaryFlags().isFragment())
		{
			cout << "generated bundle is not a fragment" << endl;
			ret = false;
		}

		if (block != NULL)
		{
			if ( !block->forFragment() )
			{
				cout << "block is not for a fragment" << endl;
				ret = false;
			}

			if ( bundle.getInteger(APPLICATION_DATA_LENGTH) != block->getFragmentLength() )
			{
				cout << "Application data size not equal: " << bundle.getInteger(APPLICATION_DATA_LENGTH) << " == " << block->getFragmentLength() << endl;
				ret = false;
			}

			if ( !block->match( bundle ) )
			{
				cout << "block-bundle match failed" << endl;
				ret = false;
			}
		}
		else	ret = false;

		delete cbundle;
		delete out;

		return ret;
	}

	bool StatusReportTestSuite::matchTest()
	{
		bool ret = true;

		// Erstellt ein Testbundle
		Bundle *b = TestUtils::createTestBundle();

		// Erstelle ein Signal für diese Bündel
		StatusReportBlock *block = PayloadBlockFactory::newStatusReportBlock();

		block->setMatch( *b );

		if (block != NULL)
		{
			if ( !block->match(*b) )
			{
				cout << "Block don't match!" << endl;
				ret = false;
			}
		}
		else
		{
			cout << "Signal not available!" << endl;
			ret = false;
		}


		delete b;
		delete block;

		return ret;
	}

	bool StatusReportTestSuite::parseTest()
	{
		BundleFactory &fac = BundleFactory::getInstance();
		dtn::data::Bundle *out = fac.newBundle();

		// Setze den Empfänger und den Absender ein
		out->setDestination( "dtn://wks1" );
		out->setSource( "dtn://wks2" );
		out->setInteger( LIFETIME, 20 );

		StatusReportBlock *signal = PayloadBlockFactory::newStatusReportBlock();
		signal->setMatch( *out );
		out->appendBlock(signal);

		unsigned char *data = out->getData();
		unsigned int length = out->getLength();

		delete out;

		out = fac.parse(data, length);

		free(data);

		signal = dynamic_cast<StatusReportBlock*>(out->getPayloadBlock());

		if ( signal == NULL )
		{
			delete out;
			return false;
		}

		if ( signal->getSource() == "" )
		{
			delete out;
			return false;
		}

		if ( out->getInteger( CREATION_TIMESTAMP ) != signal->getCreationTimestamp() )
		{
			cout << out->getInteger( CREATION_TIMESTAMP ) << " != " << signal->getCreationTimestamp() << endl;
			delete out;
			return false;
		}

		if ( out->getInteger( CREATION_TIMESTAMP_SEQUENCE ) != signal->getCreationTimestampSequence() )
		{
			cout << out->getInteger( CREATION_TIMESTAMP_SEQUENCE ) << " != " << signal->getCreationTimestampSequence() << endl;
			delete out;
			return false;
		}

		if ( out->getSource() != signal->getSource() )
		{
			cout << out->getSource() << " != " << signal->getSource() << endl;
			delete out;
			return false;
		}

		delete out;

		return true;
	}

	bool StatusReportTestSuite::createTest()
	{
		Bundle *out = TestUtils::createTestBundle(4);

		// Setze den Empfänger und den Absender ein
		out->setDestination( "dtn://wks1" );
		out->setSource( "dtn://wks2" );
		out->setInteger( LIFETIME, 20 );

		StatusReportBlock *signal = PayloadBlockFactory::newStatusReportBlock();

		if ( signal->getSource() != "dtn:none" )
		{
			delete signal;
			delete out;
			return false;
		}

		signal->setMatch(*out);

		if ( signal->getSource() != "dtn://wks2" )
		{
			delete out;
			return false;
		}

		if ( out->getInteger( CREATION_TIMESTAMP ) != signal->getCreationTimestamp() )
		{
			cout << out->getInteger( CREATION_TIMESTAMP ) << " != " << signal->getCreationTimestamp() << endl;
			delete out;
			return false;
		}

		if ( out->getInteger( CREATION_TIMESTAMP_SEQUENCE ) != signal->getCreationTimestampSequence() )
		{
			cout << out->getInteger( CREATION_TIMESTAMP_SEQUENCE ) << " != " << signal->getCreationTimestampSequence() << endl;
			delete out;
			return false;
		}

		if ( out->getSource() != signal->getSource() )
		{
			cout << out->getSource() << " != " << signal->getSource() << endl;
			delete out;
			return false;
		}

		delete signal;
		delete out;

		return true;
	}
}
}
