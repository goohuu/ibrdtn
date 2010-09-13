/*
 * Component.cpp
 *
 *  Created on: 21.04.2010
 *      Author: morgenro
 */

#include "Component.h"
#include <ibrcommon/thread/MutexLock.h>

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

			this->start();
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
