/*
 * Component.cpp
 *
 *  Created on: 21.04.2010
 *      Author: morgenro
 */

#include "Component.h"
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace daemon
	{
		Component::~Component()
		{
		}

		IndependentComponent::IndependentComponent()
		 : _running(false)
		{
		}

		IndependentComponent::~IndependentComponent()
		{
			join();
		}

		bool IndependentComponent::isRunning()
		{
			ibrcommon::MutexLock l(_running_lock);
			return _running;
		}

		void IndependentComponent::initialize()
		{
			componentUp();
		}

		void IndependentComponent::startup()
		{
			ibrcommon::MutexLock l(_running_lock);
			_running = true;

			try {
				this->start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start IndependentComponent" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void IndependentComponent::terminate()
		{
			{
				ibrcommon::MutexLock l(_running_lock);
				_running = false;
			}

			JoinableThread::stop();
			componentDown();
		}

		void IndependentComponent::run()
		{
			componentRun();

			ibrcommon::MutexLock l(_running_lock);
			_running = false;
		}

		IntegratedComponent::IntegratedComponent()
		{
		}

		IntegratedComponent::~IntegratedComponent()
		{
		}

		void IntegratedComponent::initialize()
		{
			componentUp();
		}

		void IntegratedComponent::startup()
		{
			// nothing to do
		}

		void IntegratedComponent::terminate()
		{
			componentDown();
		}
	}
}
