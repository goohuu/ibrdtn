#include "utils/Service.h"

namespace dtn
{
namespace utils
{
	Service::Service(string name)
		: m_running(false), m_started(false), m_name(name)
	{};

	Service::~Service()
	{
		abort();
	};

	void* Service::entryPoint(void *pthis)
	{
		Service* s = (Service*)pthis;
		s->run();
		return NULL;
	}

	void Service::start()
	{
		if (m_running) return;

		m_started = true;
		m_running = true;

		int ret = pthread_create( &m_thread, NULL, Service::entryPoint, this );

		if (ret == 0)
		{

		}
	}

	void Service::run()
	{
		initialize();

		while (m_running)
		{
			tick();
		}
		pthread_exit( NULL );
	}

	void Service::waitFor()
	{
		if (m_started)
		{
			m_started = false;
			pthread_join( m_thread, NULL );
		}
	}

	bool Service::isRunning()
	{
		return m_running;
	}

	void Service::abort()
	{
		finished();
		terminate();
		waitFor();
	}

	void Service::finished()
	{
		m_running = false;
	}

	string Service::getName()
	{
		return m_name;
	}
}
}
