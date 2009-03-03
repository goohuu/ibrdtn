#include "testsuite/NetworkFrameTestSuite.h"
#include "testsuite/SelfTestSuite.h"
#include "data/NetworkFrame.h"
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "utils/Utils.h"
#include "testsuite/TestUtils.h"
#include "data/SDNV.h"

using namespace dtn::data;

namespace dtn
{
namespace testsuite
{
	NetworkFrameTestSuite::NetworkFrameTestSuite()
	{
	}

	NetworkFrameTestSuite::~NetworkFrameTestSuite()
	{
	}

	bool NetworkFrameTestSuite::runAllTests()
	{
		bool ret = true;
		cout << "NetworkFrameTestSuite... ";

		if ( !instanceTest() )
		{
			cout << endl << "instanceTest() failed" << endl;
			ret = false;
		}

		if ( !sdnvTest() )
		{
			cout << endl << "sdnvTest() failed" << endl;
			ret = false;
		}

		if ( !appendDataTest() )
		{
			cout << endl << "appendDataTest() failed" << endl;
			ret = false;
		}

		if ( !copyTest() )
		{
			cout << endl << "copyTest() failed" << endl;
			ret = false;
		}

		if ( !removeTest() )
		{
			cout << endl << "removeTest() failed" << endl;
			ret = false;
		}

		if ( !insertTest() )
		{
			cout << endl << "insertTest() failed" << endl;
			ret = false;
		}

		if (ret) cout << "\t\tpassed" << endl;

		return ret;
	}

	bool NetworkFrameTestSuite::sdnvTest()
	{
		bool ret = true;

		NetworkFrame frame;
		frame.append(1);
		frame.append(288958265);
		frame.append(0);
		frame.append(288958265);
		frame.append(288958265);
		frame.append(288958265);

		NetworkFrame frame2;
		unsigned char *data = frame.get(0);

		for (int i = 0; i < 6; i++)
		{
			size_t len = SDNV::len(data);
			unsigned int value = 0;
			SDNV::decode(data, len, &value);

			switch (i)
			{
				case 0:
					if (value != 1 )
					{
						ret = false;
					}
					break;

				case 2:
					if (value != 0 )
					{
						ret = false;
					}
					break;
				default:
					if (value != 288958265 )
					{
						ret = false;
					}
				break;
			}

			data += len;
		}

		return ret;
	}

	bool NetworkFrameTestSuite::insertTest()
	{
		bool ret = true;

		NetworkFrame frame;

		frame.append('A');
		frame.append('B');
		frame.append('C');

		frame.insert(2);
		frame.set(2, 1024);

		if (frame.getChar(0) != 'A')
		{
			ret = false;
		}

		if (frame.getChar(1) != 'B')
		{
			ret = false;
		}

		if (frame.getChar(2) == 'C')
		{
			ret = false;
		}

		if (frame.getSDNV(2) != 1024)
		{
			ret = false;
		}

		if (frame.getChar(3) != 'C')
		{
			ret = false;
		}

		return ret;
	}

	bool NetworkFrameTestSuite::removeTest()
	{
		bool ret = true;

		NetworkFrame frame;

		frame.append('A');
		frame.append('B');
		frame.append(1024);
		frame.append('C');
		frame.append(2);

		frame.remove(0);

		if (frame.getChar(0) != 'B')
		{
			ret = false;
		}

		if (frame.getSDNV(1) != 1024)
		{
			ret = false;
		}

		frame.remove(1);

		if (frame.getSDNV(2) != 2)
		{
			ret = false;
		}

		return ret;
	}

	bool NetworkFrameTestSuite::copyTest()
	{
		bool ret = true;

		NetworkFrame frame1;

		frame1.append('A');
		frame1.append('B');
		frame1.append('C');
		frame1.append(1024);

		// copy the frame
		NetworkFrame frame2 = frame1;

		frame1.set(2, 'Z');

		if (TestUtils::compareData(frame1.getData(), frame2.getData(), frame1.getSize()))
		{
			ret = false;
		}

		return ret;
	}

	bool NetworkFrameTestSuite::appendDataTest()
	{
		// Einzufügende Daten
		string write_str = "012345";

		// Instantiiert ein leeres NetworkFrame-Objekt
		NetworkFrame frame;

		// Daten anfügen, Feld: 0
		frame.append( (unsigned char*)write_str.data(), write_str.length() );

		// Weitere einzufügende Daten
		write_str = "6789";

		// Daten anfügen, Feld: 1
		frame.append( (unsigned char*)write_str.data(), write_str.length() );

		// Die Datenlänge des NetworkFrames müsste nun 10 Byte betragen, falls nicht melde: Fehlgeschlagen
		if (frame.getSize() != 10) return false;

		// Daten zur Überprüfung in eine Zeichenkette umwandeln
		char data[11];
		memcpy(&data, frame.getData(), 10);
		data[10] = '\0';

		// Wenn Zeichenkette wie erwartet => Test erfolgreich
		if ((string)data == "0123456789") return true;

		// Wenn nicht erfolgreich false zurückgeben
		return false;
	}

	bool NetworkFrameTestSuite::instanceTest()
	{
		NetworkFrame frame;

		// Test String
		string teststr1 = "Hallo Welt!";
		string teststr2 = "Ich bin da!";
		string teststr3 = "Test. Hallo Welt! ";
		string tmpstr;

		char *data1 = new char [ teststr1.size() + 1 ];
		char *data2 = new char [ teststr2.size() + 1 ];
		char *data3 = new char [ teststr3.size() + 1 ];

		strcpy (data1, teststr1.c_str());
		strcpy (data2, teststr2.c_str());
		strcpy (data3, teststr3.c_str());

		frame.append( reinterpret_cast<unsigned char*>(data1), teststr1.size() + 1 );
		frame.append( reinterpret_cast<unsigned char*>(data2), teststr2.size() + 1 );

		char *stringdata = reinterpret_cast<char*>( frame.get(0) );
		tmpstr = stringdata;

		if (tmpstr != teststr1) return false;

		frame.set(0, reinterpret_cast<unsigned char*>(data3), teststr3.size() + 1 );

		stringdata = reinterpret_cast<char*>(frame.get(0));
		tmpstr = stringdata;

		if (tmpstr != teststr3) return false;

		stringdata = reinterpret_cast<char*>(frame.get(1));
		tmpstr = stringdata;

		if (tmpstr != teststr2) return false;

		frame.set(1, reinterpret_cast<unsigned char*>(data3), teststr3.size() + 1 );

		stringdata = reinterpret_cast<char*>(frame.get(1));
		tmpstr = stringdata;

		if (tmpstr != teststr3) return false;

		delete[] data1;
		delete[] data2;
		delete[] data3;

		return true;
	}
}
}
