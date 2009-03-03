#include "testsuite/MeasurementTestSuite.h"

#ifdef USE_EMMA_CODE

#include "emma/GPSDummy.h"
#include "emma/Measurement.h"
#include <iostream>
#include <fstream>
#include "utils/Utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;
using namespace emma;

namespace dtn
{
	namespace testsuite
	{
		MeasurementTestSuite::MeasurementTestSuite()
		{
		}

		MeasurementTestSuite::~MeasurementTestSuite()
		{
		}

		bool MeasurementTestSuite::runAllTests()
		{
			bool ret = true;
			cout << "MeasurementTestSuite... ";

//			if ( !createData() )
//			{
//				cout << endl << "createData() failed" << endl;
//				ret = false;
//			}

			if (ret) cout << "\t\t\tpassed" << endl;

			return ret;
		}

		bool MeasurementTestSuite::createData()
		{
			srand ( time(NULL) );
			fstream f( "measurement.dat", ios_base::out );

			double lat = 52.261193;
			double lon = 10.520240;
			double value = 50;

			for (int i = 0; i < 10000; i++)
			{
				// Verändere die Werte zufällig
				// GPS Veränderungen im Bereich von -0.001 bis +0.001
				double r = (rand() % 200) - 100;
				lat = lat + (r / 100000);

				r = (rand() % 200) - 100;
				lon = lon + (r / 100000);

				// Messwerte im Bereich von -2 bis +2
				if (value > 98)
				{
					value = value + ((rand() % 5) - 5);
				}
				else if (value < 2)
				{
					value = value + (rand() % 5);
				}
				else
				{
					value = value + ((rand() % 5) - 2);
				}

				// Erstelle ein Fake GPS
				GPSDummy gps(lat, lon);

				// Messdatensatz erstellen
				Measurement m(0, 2);

				// GPS Koordination hinzufügen
				m.add(&gps);

				// Messwert hinzufügen
				m.add(3, value);

				f.write( (const char*)m.getData(), m.getLength() );
			}

			return true;
		}
	}
}

#endif
