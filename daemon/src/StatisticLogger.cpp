/*
 * StatisticLogger.cpp
 *
 *  Created on: 05.05.2010
 *      Author: morgenro
 */

#include "StatisticLogger.h"
#include "net/BundleReceivedEvent.h"
#include "net/TransferCompletedEvent.h"
#include <ibrdtn/utils/Utils.h>
#include <typeinfo>

namespace dtn
{
	namespace daemon
	{
		StatisticLogger::StatisticLogger(LoggerType type, unsigned int interval)
		 : _timer(*this, 0), _type(type), _interval(interval), _sentbundles(0), _recvbundles(0),
		   _core(dtn::core::BundleCore::getInstance())
		{
			std::cout << "Logging module initialized. mode = " << _type << ", interval = " << interval << std::endl;
		}

		StatisticLogger::StatisticLogger(LoggerType type, unsigned int interval, std::string filename)
		 : _timer(*this, 0), _type(type), _interval(interval), _file(filename), _sentbundles(0), _recvbundles(0),
		   _core(dtn::core::BundleCore::getInstance())
		{
		}

		StatisticLogger::~StatisticLogger()
		{

		}

		void StatisticLogger::componentUp()
		{
			if ((_type == LOGGER_FILE_PLAIN) || (_type == LOGGER_FILE_CVS))
			{
				// open statistic file
				if (_file.exists())
				{
					_fileout.open(_file.getPath().c_str(), ios_base::app);
				}
				else
				{
					_fileout.open(_file.getPath().c_str(), ios_base::trunc);
				}
			}

			// start the timer
			_timer.set(1);
			_timer.start();

			// register at the events
			bindEvent(dtn::net::BundleReceivedEvent::className);
			bindEvent(dtn::net::TransferCompletedEvent::className);
		}

		size_t StatisticLogger::timeout(size_t identifier)
		{
			switch (_type)
			{
			case LOGGER_STDOUT:
				writeStdLog(std::cout);
				break;

			case LOGGER_SYSLOG:
				writeSyslog(std::cout);
				break;

			case LOGGER_FILE_PLAIN:
				writePlainLog(_fileout);
				break;

			case LOGGER_FILE_CVS:
				writeCvsLog(_fileout);
				break;

			case LOGGER_FILE_STAT:
				writeStatLog();
				break;
			}

			return _interval;
		}

		void StatisticLogger::componentDown()
		{
			unbindEvent(dtn::net::BundleReceivedEvent::className);
			unbindEvent(dtn::net::TransferCompletedEvent::className);

			_timer.remove();

			if (_type >= LOGGER_FILE_PLAIN)
			{
				// close the statistic file
				_fileout.close();
			}
		}

		void StatisticLogger::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::net::BundleReceivedEvent &recvd = dynamic_cast<const dtn::net::BundleReceivedEvent&>(*evt);
				_recvbundles++;
			} catch (std::bad_cast& bc) {
			}

			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);
				_sentbundles++;
			} catch (std::bad_cast& bc) {
			}
		}

		void StatisticLogger::writeSyslog(std::ostream &stream)
		{
			const std::list<dtn::core::Node> neighbors = _core.getNeighbors();
			size_t timestamp = dtn::utils::Utils::get_current_dtn_time();
			dtn::core::BundleStorage &storage = _core.getStorage();

			stream	<< "DTN-Stats: CurNeighbors " << neighbors.size()
					<< "; RecvBundles " << _recvbundles
					<< "; SentBundles " << _sentbundles
					<< "; StoredBundles " << storage.count()
					<< ";" << std::endl;
		}

		void StatisticLogger::writeStdLog(std::ostream &stream)
		{
			const std::list<dtn::core::Node> neighbors = _core.getNeighbors();
			size_t timestamp = dtn::utils::Utils::get_current_dtn_time();
			dtn::core::BundleStorage &storage = _core.getStorage();

			stream	<< "Timestamp " << timestamp
					<< "; CurNeighbors " << neighbors.size()
					<< "; RecvBundles " << _recvbundles
					<< "; SentBundles " << _sentbundles
					<< "; StoredBundles " << storage.count()
					<< ";" << std::endl;
		}

		void StatisticLogger::writePlainLog(std::ostream &stream)
		{
			const std::list<dtn::core::Node> neighbors = _core.getNeighbors();
			size_t timestamp = dtn::utils::Utils::get_current_dtn_time();
			dtn::core::BundleStorage &storage = _core.getStorage();

			stream 	<< timestamp << " "
					<< neighbors.size() << " "
					<< _recvbundles << " "
					<< _sentbundles << " "
					<< storage.count() << std::endl;
		}

		void StatisticLogger::writeCvsLog(std::ostream &stream)
		{
			const std::list<dtn::core::Node> neighbors = _core.getNeighbors();
			size_t timestamp = dtn::utils::Utils::get_current_dtn_time();
			dtn::core::BundleStorage &storage = _core.getStorage();

			stream 	<< timestamp << ","
					<< neighbors.size() << ","
					<< _recvbundles << ","
					<< _sentbundles << ","
					<< storage.count() << std::endl;
		}

		void StatisticLogger::writeStatLog()
		{
			// open statistic file
			_fileout.open(_file.getPath().c_str(), ios_base::trunc);

			const std::list<dtn::core::Node> neighbors = _core.getNeighbors();
			size_t timestamp = dtn::utils::Utils::get_current_dtn_time();
			dtn::core::BundleStorage &storage = _core.getStorage();

			_fileout << "IBR-DTN statistics" << std::endl;
			_fileout << std::endl;
			_fileout << "timestamp: " << timestamp << std::endl;
			_fileout << "bundles sent: " << _sentbundles << std::endl;
			_fileout << "bundles received: " << _recvbundles << std::endl;
			_fileout << std::endl;
			_fileout << "bundles in storage: " << storage.count() << std::endl;
			_fileout << std::endl;
			_fileout << "neighbors (" << neighbors.size() << "):" << std::endl;

			for (std::list<dtn::core::Node>::const_iterator iter = neighbors.begin(); iter != neighbors.end(); iter++)
			{
				_fileout << "\t" << (*iter).getURI() << std::endl;
			}

			_fileout << std::endl;

			// close the file
			_fileout.close();
		}
	}
}
