/*
 * StatisticLogger.h
 *
 *  Created on: 05.05.2010
 *      Author: morgenro
 *
 *
 * The Statistic Logger generate a statistic about:
 *  - Current Neighbors
 *  - Recv Bundles
 *  - Sent Bundles
 *  - Bundles in Storage
 *
 * This data is logged into a file. The format for this file
 * can specified with the "format"-variable.
 *
 */

#ifndef STATISTICLOGGER_H_
#define STATISTICLOGGER_H_

#include "Component.h"
#include "core/EventReceiver.h"
#include "core/Node.h"
#include "core/BundleCore.h"
#include <ibrcommon/data/File.h>
#include <ibrcommon/thread/Timer.h>
#include <list>
#include <string>
#include <fstream>
#include <iostream>

namespace dtn
{
	namespace daemon
	{
		class StatisticLogger : public IntegratedComponent, public ibrcommon::SimpleTimerCallback, public dtn::core::EventReceiver
		{
		public:
			enum LoggerType
			{
				LOGGER_STDOUT = 0,		// All output is human readable and directed into stdout.
				LOGGER_SYSLOG = 1,		// All output is human readable and directed into the syslog.
				LOGGER_FILE_PLAIN = 10,	// All output is machine readable only and appended into a file.
				LOGGER_FILE_CSV = 11,	// All output is machine readable only and appended into a csv file.
				LOGGER_FILE_STAT = 12	// All output is machine readable only and directed into a stat file. This file contains only one dataset.
			};

			StatisticLogger(LoggerType type, unsigned int interval);
			StatisticLogger(LoggerType type, unsigned int interval, ibrcommon::File file);
			virtual ~StatisticLogger();

			void componentUp();
			void componentDown();

			size_t timeout(size_t identifier);
			void raiseEvent(const dtn::core::Event *evt);

		private:
			void writeStdLog(std::ostream &stream);
			void writeSyslog(std::ostream &stream);
			void writePlainLog(std::ostream &stream);
			void writeCsvLog(std::ostream &stream);
			void writeStatLog();

			ibrcommon::SimpleTimer _timer;
			ibrcommon::File _file;
			std::ofstream _fileout;
			LoggerType _type;
			size_t _interval;

			size_t _sentbundles;
			size_t _recvbundles;

			dtn::core::BundleCore &_core;
		};
	}
}

#endif /* STATISTICLOGGER_H_ */
