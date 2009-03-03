#ifndef TCPMESSAGEHEADER_H_
#define TCPMESSAGEHEADER_H_

#include "data/Exceptions.h"
#include "data/NetworkFrame.h"
#include <bitset>

using namespace std;

namespace dtn
{
	namespace exceptions
	{
		class IncompleteDataException : public Exception
		{
			public:
				IncompleteDataException() : Exception("Data incomplete")
				{
				};

				IncompleteDataException(string what) : Exception(what)
				{
				};
		};
	}
}

namespace dtn
{
	namespace core
	{
		enum TCPMessageType
		{
			MSG_DATA_SEGMENT = 0x01,
			MSG_ACK_SEGMENT = 0x02,
			MSG_REFUSE_BUNDLE = 0x03,
			MSG_KEEPALIVE = 0x04,
			MSG_SHUTDOWN = 0x05
		};

		enum TCPMessageShutdownReason
		{
			MSG_SHUTDOWN_NONE = 0x00,
			MSG_SHUTDOWN_IDLE_TIMEOUT = 0x01,
			MSG_SHUTDOWN_VERSION_MISSMATCH = 0x02,
			MSG_SHUTDOWN_BUSY = 0x03
		};

		class TCPMessage
		{
			public:
				TCPMessage(size_t chunksize, char *data); // creates a Data Message
				TCPMessage(size_t acksize); // creates a Ack Message
				TCPMessage(); // Creates a Keep-Alive Message
				TCPMessage(TCPMessageShutdownReason reason = MSG_SHUTDOWN_NONE, size_t reconnect = 0);  // Creates a Shutdown Message

				//TCPMessage(char *data) throw (dtn::exceptions::IncompleteDataException);
				~TCPMessage();

				char* getData();
				size_t getSize();
				void setFirst(bool value);
				void setLast(bool value);
			private:
				dtn::data::NetworkFrame *m_frame;
		};
	}
}

#endif /*TCPMESSAGEHEADER_H_*/
