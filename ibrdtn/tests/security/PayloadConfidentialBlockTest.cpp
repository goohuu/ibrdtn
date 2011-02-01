/*
 * PayloadConfidentialBlockTest.cpp
 *
 *  Created on: 01.02.2011
 *      Author: morgenro
 */

#include "security/PayloadConfidentialBlockTest.h"
#include <ibrdtn/security/PayloadConfidentialBlock.h>
#include <ibrdtn/security/SecurityKey.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/EID.h>
#include <ibrdtn/data/PayloadBlock.h>
#include <ibrcommon/data/File.h>

#include <cppunit/extensions/HelperMacros.h>
#include <sstream>

CPPUNIT_TEST_SUITE_REGISTRATION (PayloadConfidentialBlockTest);


void PayloadConfidentialBlockTest::setUp(void)
{
}

void PayloadConfidentialBlockTest::tearDown(void)
{
}

void PayloadConfidentialBlockTest::encryptTest(void)
{
	dtn::security::SecurityKey pubkey;
	pubkey.type = dtn::security::SecurityKey::KEY_PUBLIC;
	pubkey.file = ibrcommon::File("test-key.pem");
	pubkey.reference = dtn::data::EID("dtn://test");

	dtn::security::SecurityKey pkey;
	pkey.type = dtn::security::SecurityKey::KEY_PRIVATE;
	pkey.file = ibrcommon::File("test-key.pem");

	if (!pubkey.file.exists())
	{
		throw ibrcommon::Exception("test-key.pem file not exists!");
	}

	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://test");
	b._destination = pubkey.reference;

	// add payload block
	dtn::data::PayloadBlock &p = b.push_back<dtn::data::PayloadBlock>();

	// write some payload
	(*p.getBLOB().iostream()) << "Hallo Welt!" << std::flush;

	// verify the payload
	{
		std::stringstream ss; ss << (*p.getBLOB().iostream()).rdbuf();

		if (ss.str() != "Hallo Welt!")
		{
			throw ibrcommon::Exception("payload write failed!");
		}
	}

	// encrypt the payload block
	dtn::security::PayloadConfidentialBlock::encrypt(b, pubkey, b._source);

	// verify the encryption
	{
		std::stringstream ss; ss << (*p.getBLOB().iostream()).rdbuf();

		if (ss.str() == "Hallo Welt!")
		{
			throw ibrcommon::Exception("encryption failed!");
		}
	}
}

void PayloadConfidentialBlockTest::decryptTest(void)
{
	dtn::security::SecurityKey pubkey;
	pubkey.type = dtn::security::SecurityKey::KEY_PUBLIC;
	pubkey.file = ibrcommon::File("test-key.pem");
	pubkey.reference = dtn::data::EID("dtn://test");

	dtn::security::SecurityKey pkey;
	pkey.type = dtn::security::SecurityKey::KEY_PRIVATE;
	pkey.file = ibrcommon::File("test-key.pem");

	if (!pubkey.file.exists())
	{
		throw ibrcommon::Exception("test-key.pem file not exists!");
	}

	dtn::data::Bundle b;
	b._source = dtn::data::EID("dtn://test");
	b._destination = pubkey.reference;

	// add payload block
	dtn::data::PayloadBlock &p = b.push_back<dtn::data::PayloadBlock>();

	// encrypt the payload block
	dtn::security::PayloadConfidentialBlock::encrypt(b, pubkey, b._source);

	// decrypt the payload
	dtn::security::PayloadConfidentialBlock::decrypt(b, pkey);

	// verify the payload
	{
		std::stringstream ss; ss << (*p.getBLOB().iostream()).rdbuf();

		if (ss.str() != "Hallo Welt!")
		{
			throw ibrcommon::Exception("decryption failed!");
		}
	}
}
