#ifndef TESTSERVICE_H_
#define TESTSERVICE_H_

#include "utils/Service.h"
#include <string>

namespace dtn
{
namespace testsuite
{
	class TestService : public dtn::utils::Service
	{
		public:
		TestService(std::string msg);
		~TestService();

		virtual void tick();
		bool worked();

		private:
		bool m_worked;
	};
}
}

#endif /*TESTSERVICE_H_*/
