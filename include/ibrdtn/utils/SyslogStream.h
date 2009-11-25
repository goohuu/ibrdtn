/*
 * File:   SyslogStream.h
 * Author: morgenro
 *
 * Created on 17. November 2009, 13:43
 */

#ifndef _SYSLOGSTREAM_H
#define	_SYSLOGSTREAM_H

#include "ibrdtn/default.h"
#include <vector>
#include <syslog.h>

namespace dtn
{
    namespace utils
    {
        //#define slog dtn::utils::SyslogStream::getStream()

        enum SyslogPriority
        {
            SYSLOG_EMERG =	LOG_EMERG,	/* system is unusable */
            SYSLOG_ALERT =	LOG_ALERT,	/* action must be taken immediately */
            SYSLOG_CRIT =	LOG_CRIT,	/* critical conditions */
            SYSLOG_ERR =	LOG_ERR,	/* error conditions */
            SYSLOG_WARNING =    LOG_WARNING,	/* warning conditions */
            SYSLOG_NOTICE =	LOG_NOTICE,	/* normal but significant condition */
            SYSLOG_INFO =	LOG_INFO,	/* informational */
            SYSLOG_DEBUG =	LOG_DEBUG	/* debug-level messages */
        };

        std::ostream &operator<<(std::ostream &stream, const SyslogPriority &prio);

        class SyslogStream : public std::streambuf
        {
        private:
            enum { BUF_SIZE = 256 };

        public:
            SyslogStream();
            virtual ~SyslogStream();

            static std::ostream& getStream();

            void setPriority(const SyslogPriority &prio);

        protected:
            virtual int sync();

        private:
            void writeToDevice();
            std::vector< char_type > m_buf;
            SyslogPriority _prio;
        };

        extern std::ostream &slog;
    }
}

#endif	/* _SYSLOGSTREAM_H */

