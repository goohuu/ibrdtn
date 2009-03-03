#include "core/TCPMessage.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "data/SDNV.h"

using namespace std;

namespace dtn
{
	namespace core
	{
		TCPMessage::TCPMessage(size_t chunksize, char *data) : m_frame(NULL)
		{
			m_frame = new dtn::data::NetworkFrame();
			m_frame->append((unsigned char)MSG_DATA_SEGMENT);
			m_frame->append((u_int64_t)chunksize);
			m_frame->append((unsigned char*)data, chunksize);
		}

		TCPMessage::TCPMessage(size_t acksize) : m_frame(NULL)
		{
			m_frame = new dtn::data::NetworkFrame();
			m_frame->append((unsigned char)MSG_ACK_SEGMENT);
			m_frame->append((u_int64_t)acksize);
		}

		TCPMessage::TCPMessage() : m_frame(NULL)
		{
			m_frame = new dtn::data::NetworkFrame();
			m_frame->append((unsigned char)MSG_KEEPALIVE);
		}

		TCPMessage::TCPMessage(TCPMessageShutdownReason reason, size_t reconnect) : m_frame(NULL)
		{
			m_frame = new dtn::data::NetworkFrame();
			m_frame->append((unsigned char)MSG_SHUTDOWN);

			if (reason != MSG_SHUTDOWN_NONE)
			{
				m_frame->append((u_int64_t)reason);
			}

			if (reconnect != 0)
			{
				m_frame->append((u_int64_t)reconnect);
			}
		}

		TCPMessage::~TCPMessage()
		{
			if (m_frame != NULL) delete m_frame;
		}

		size_t TCPMessage::getSize()
		{
			return m_frame->getSize();
		}

		void TCPMessage::setLast(bool value)
		{
			unsigned char data = m_frame->getChar(0);

			if (value)
				data |= 128; // mask 00000001 = 128
			else
				data &= 127; // mask 11111110 = 127

			m_frame->set(0, data);
		}

		void TCPMessage::setFirst(bool value)
		{
			unsigned char data = m_frame->getChar(0);

			if (value)
				data |= 64; // mask 00000010 = 64
			else
				data &= 191; // mask 11111101 = 191

			m_frame->set(0, data);
		}

		char* TCPMessage::getData()
		{
			return (char*)m_frame->getData();
		}
	}
};
