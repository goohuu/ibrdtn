#include "testsuite/TestService.h"
#include "utils/Service.h"
#include "utils/Utils.h"

namespace dtn
{
namespace testsuite
{
	TestService::TestService(std::string msg) : Service("TestService")
	{
		m_worked = false;
	}

	TestService::~TestService()
	{}

	void TestService::tick()
	{
		m_worked = true;
		Service::finished();
	}

	bool TestService::worked()
	{
		return m_worked;
	}
}
}
