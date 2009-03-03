#include "utils/Utils.h"
#include <math.h>

using namespace std;

namespace dtn
{
	namespace utils
	{
		vector<string> Utils::tokenize(string token, string data)
		{
			vector<string> l;
			string value;

			// Skip delimiters at beginning.
			string::size_type lastPos = data.find_first_not_of(token, 0);
			// Find first "non-delimiter".
			string::size_type pos     = data.find_first_of(token, lastPos);

			while (string::npos != pos || string::npos != lastPos)
			{
				// Found a token, add it to the vector.
				value = data.substr(lastPos, pos - lastPos);
				l.push_back(value);
				// Skip delimiters.  Note the "not_of"
				lastPos = data.find_first_not_of(token, pos);
				// Find next "non-delimiter"
				pos = data.find_first_of(token, lastPos);
			}

			return l;
		}

		/**
		 * Berechnet die Distanz zwischen zwei Punkten (Latitude, Longitude)
		 */
		 double Utils::distance(double lat1, double lon1, double lat2, double lon2)
		 {
			const double r = 6371; //km

			double dLat = toRad( (lat2-lat1) );
			double dLon = toRad( (lon2-lon1) );

			double a = 	sin(dLat/2) * sin(dLat/2) +
						cos(toRad(lat1)) * cos(toRad(lat2)) *
						sin(dLon/2) * sin(dLon/2);
			double c = 2 * atan2(sqrt(a), sqrt(1-a));
			return r * c;
		 }

		 const double Utils::pi = 3.14159;

		 double Utils::toRad(double value)
		 {
			return value * pi / 180;
		 }
	}
}
