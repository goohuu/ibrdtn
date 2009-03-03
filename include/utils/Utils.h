#ifndef UTILS_H_
#define UTILS_H_

#include <vector>
#include <string>
using namespace std;

namespace dtn
{
	namespace utils
	{
		class Utils
		{
			public:
			static vector<string> tokenize(string token, string data);
			static double distance(double lat1, double lon1, double lat2, double lon2);

			private:
			static double toRad(double value);
			static const double pi;
		};
	}
}

#endif /*UTILS_H_*/
