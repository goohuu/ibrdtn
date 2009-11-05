#ifndef UTILS_H_
#define UTILS_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/PayloadBlock.h"

namespace dtn
{
	namespace utils
	{
		class Utils
		{
		public:
			static vector<string> tokenize(string token, string data);
			static double distance(double lat1, double lon1, double lat2, double lon2);
			static size_t get_current_dtn_time();

			static dtn::data::CustodySignalBlock* getCustodySignalBlock(const dtn::data::Bundle &bundle);
			static dtn::data::StatusReportBlock* getStatusReportBlock(const dtn::data::Bundle &bundle);
			static dtn::data::PayloadBlock* getPayloadBlock(const dtn::data::Bundle &bundle);

			static pair<dtn::data::PayloadBlock*, dtn::data::PayloadBlock*> split(dtn::data::PayloadBlock *block, size_t split_position);
			static bool compareFragments(const dtn::data::Bundle &first, const dtn::data::Bundle &second);
			static dtn::data::Bundle merge(dtn::data::Bundle &destination, const dtn::data::Bundle &source);
			static dtn::data::Bundle merge(std::list<dtn::data::Bundle> &bundles);

			static int timezone;

		private:
			static double toRad(double value);
			static const double pi;
		};
	}
}

#endif /*UTILS_H_*/
