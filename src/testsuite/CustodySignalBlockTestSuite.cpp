#include "testsuite/CustodySignalBlockTestSuite.h"
#include "testsuite/SelfTestSuite.h"
#include "testsuite/TestUtils.h"
#include "data/Bundle.h"
#include "data/BundleFactory.h"
#include "data/PayloadBlockFactory.h"
#include "data/CustodySignalBlock.h"
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
	CustodySignalBlockTestSuite::CustodySignalBlockTestSuite()
	{
	}

	CustodySignalBlockTestSuite::~CustodySignalBlockTestSuite()
	{
	}

	bool CustodySignalBlockTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "CustodySignalBlockTestSuite... ";

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

	bool CustodySignalBlockTestSuite::fragmentTest()
	{
		bool ret = true;

		Bundle *out = TestUtils::createTestBundle(1000);
		list<Bundle*> bundles = BundleFactory::split(*out, 200);

		Bundle *bundle = bundles.back();
		BundleFactory &fac = BundleFactory::getInstance();
		Bundle *cbundle = fac.newBundle();
		CustodySignalBlock *block = PayloadBlockFactory::newCustodySignalBlock( true );
		cbundle->appendBlock( block );

		block->setMatch( *bundle );

		if (!bundle->getPrimaryFlags().isFragment())
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

			if ( bundle->getInteger(APPLICATION_DATA_LENGTH) != block->getFragmentLength() )
			{
				cout << "Application data size not equal: " << bundle->getInteger(APPLICATION_DATA_LENGTH) << " == " << block->getFragmentLength() << endl;
				ret = false;
			}

			if ( !block->match( *bundle ) )
			{
				cout << "block-bundle match failed" << endl;
				ret = false;
			}
		}
		else	ret = false;

		delete cbundle;

		// Aufräumen
		list<Bundle*>::iterator iter = bundles.begin();

		while ( iter != bundles.end() )
		{
			delete (*iter);
			iter++;
		}

		delete out;

		return ret;
	}

	bool CustodySignalBlockTestSuite::matchTest()
	{
		bool ret = true;

		// Erstellt ein Testbundle
		Bundle *b = TestUtils::createTestBundle();

		// Erstelle ein Signal für diese Bündel
		CustodySignalBlock *signal = PayloadBlockFactory::newCustodySignalBlock(true);

		signal->setMatch( *b );

		if (signal != NULL)
		{
			if ( !signal->match(*b) )
			{
				cout << "Block don't match!" << endl;
				ret = false;
			}

			if ( !signal->isAccepted() )
			{
				cout << "Signal not correct!" << endl;
				ret = false;
			}
		}
		else
		{
			cout << "Signal not available!" << endl;
			ret = false;
		}


		delete b;
		delete signal;

		return ret;
	}

	bool CustodySignalBlockTestSuite::parseTest()
	{
		BundleFactory &fac = BundleFactory::getInstance();
		dtn::data::Bundle *out = fac.newBundle();

		// Setze den Empfänger und den Absender ein
		out->setDestination( "dtn://wks1" );
		out->setSource( "dtn://wks2" );
		out->setInteger( LIFETIME, 20 );

		CustodySignalBlock *signal = PayloadBlockFactory::newCustodySignalBlock( true );
		signal->setMatch( *out );
		out->appendBlock(signal);

		unsigned char *data = out->getData();
		unsigned int length = out->getLength();

		delete out;

		out = fac.parse(data, length);

		free(data);

		signal = dynamic_cast<CustodySignalBlock*>(out->getPayloadBlock());

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

	bool CustodySignalBlockTestSuite::createTest()
	{
		Bundle *out = TestUtils::createTestBundle(4);

		// Setze den Empfänger und den Absender ein
		out->setDestination( "dtn://wks1" );
		out->setSource( "dtn://wks2" );
		out->setInteger( LIFETIME, 20 );

		CustodySignalBlock *signal = PayloadBlockFactory::newCustodySignalBlock( true );

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
