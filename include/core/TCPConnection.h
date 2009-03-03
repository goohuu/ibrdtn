#ifndef TCPCONNECTION_H_
#define TCPCONNECTION_H_

#include "data/Bundle.h"
#include "core/ConvergenceLayer.h"
#include "core/TCPConvergenceLayer.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include "utils/Conditional.h"

using namespace dtn::data;
using namespace std;


namespace dtn
{
	namespace core
	{
		enum TCPConnectionState
		{
			STATE_INITIAL = 0,
			STATE_HANDSHAKE = 1,
			STATE_IDLE = 2,
			STATE_MESSAGE = 3,
			STATE_TRANSMITTING = 4,
			STATE_TRANSFER_COMPLETE = 5,
			STATE_CLOSED = 6
		};

		struct TCPConnectionData
		{
			unsigned int payloadsize;
			char* data;
			unsigned int length;
			char* position;
			unsigned int remain;
			Mutex mutex;
			bool last;
			unsigned int idletime;
			u_int16_t keepalive;
			unsigned int ackdata;

			TCPConnectionState state;
			Conditional state_cond;
			Mutex mutex_state;
			bool outbound;
		};

		/**
		 * Diese Klasse verwaltet eine TCP/IP Verbindung
		 */
		class TCPConnection
		{
		friend class TCPConvergenceLayer;

		public:
			/**
			 * Konstruktor
			 * @param[in] clayer The parent TCPConvergenceLayer which this Connection belongs to.
			 * @param[in] socket The socket to work with.
			 * @param[in] chunksize The default chunk size for data transmission.
			 */
			TCPConnection(TCPConvergenceLayer &clayer, int socket, size_t chunksize = 128);

			/**
			 * Desktruktor
			 */
			~TCPConnection();

			string getRemoteURI();

			/**
			 * returns true if the connection is closed
			 */
			bool isClosed();

			TCPConnectionState getState();

			bool waitForState(TCPConnectionState state);

		protected:
			/**
			 * transmit some data and block while transmitting
			 */
			unsigned int transmit(char *data, unsigned int size);

			/**
			 * this function returns true if there is data to send
			 */
			bool datawaiting();

			/**
			 * transmit waiting data
			 */
			void transmit();

			/**
			 * receive waiting data
			 */
			void receive();

			int getSocket();

			/**
			 * send the contact header
			 * @param data a data array which contains the contact header
			 * @param size the size of the data array
			 */
			void handshake(unsigned char* data, unsigned int size);

			/**
			 * checks for idled connections
			 */
			void idlechecking();

		private:
			void disconnect();
			void setState(TCPConnectionState state);

			void setState(TCPConnectionData &data, TCPConnectionState state);
			TCPConnectionState getState(TCPConnectionData &data);
			bool waitForState(TCPConnectionData &data, TCPConnectionState state);

			int m_socket;
			TCPConvergenceLayer &m_clayer;
			string m_eid;

			TCPConnectionData m_incoming;
			TCPConnectionData m_outgoing;

			Mutex m_mutex_state;
			TCPConnectionState m_state;
			Conditional m_cond_state;

			size_t m_chunksize;
		};
	}
}

#endif /*TCPCONNECTION_H_*/
