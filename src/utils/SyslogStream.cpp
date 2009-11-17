/* 
 * File:   SyslogStream.cpp
 * Author: morgenro
 * 
 * Created on 17. November 2009, 13:43
 */

#include "ibrdtn/utils/SyslogStream.h"

namespace dtn
{
    namespace utils
    {
        SyslogStream syslogbuf;
        std::iostream syslog(&syslogbuf);

        SyslogStream::SyslogStream() : m_buf( BUF_SIZE+1 ), _prio(SYSLOG_INFO)
        {
            setp( &m_buf[0], &m_buf[0] + (m_buf.size()-1) );
        }

        SyslogStream::~SyslogStream() {
        }

        int SyslogStream::sync()
        {
            if( pptr() > pbase() )
                writeToDevice();
            return 0; // 0 := Ok
        }

        void SyslogStream::setPriority(const SyslogPriority &prio)
        {
            _prio = prio;
        }

        void SyslogStream::writeToDevice()
        {
            *pptr() = 0; // End Of String
            ::syslog( _prio, pbase() );
            setp( &m_buf[0], &m_buf[0] + (m_buf.size()-1) );
        }

        std::ostream &operator<<(std::ostream &stream, const SyslogPriority &prio)
        {
            SyslogStream *slog = dynamic_cast<SyslogStream*>(stream.rdbuf());

            if (slog != NULL)
            {
                slog->setPriority(prio);
            }
            else
            {
                const char data = prio;
                stream.write(&data, sizeof(char));
            }

            return stream;
        }
    }
}