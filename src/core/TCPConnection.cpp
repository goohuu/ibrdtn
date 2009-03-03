#include "core/TCPConnection.h"
#include "core/TCPMessage.h"
#include "data/BundleFactory.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "utils/Utils.h"
#include "data/SDNV.h"


using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		TCPConnection::TCPConnection(TCPConvergenceLayer &clayer, int socket, size_t chunksize)
			: m_socket(socket), m_clayer(clayer), m_eid("dtn:none"), m_state(STATE_INITIAL), m_chunksize(chunksize)
		{
			m_outgoing.data = NULL;
			m_outgoing.length = 0;
			m_outgoing.position = NULL;
			m_outgoing.remain = 0;
			m_outgoing.state = STATE_INITIAL;
			m_outgoing.outbound = true;
			m_outgoing.idletime = 0;
			m_outgoing.ackdata = 0;

			m_incoming.data = NULL;
			m_incoming.length = 0;
			m_incoming.position = NULL;
			m_incoming.remain = 0;
			m_incoming.state = STATE_INITIAL;
			m_incoming.outbound = false;
			m_incoming.idletime = 0;
			m_outgoing.ackdata = 0;
		}

		TCPConnection::~TCPConnection()
		{
			disconnect();
		}

		TCPConnectionState TCPConnection::getState(TCPConnectionData &data)
		{
			MutexLock l(data.mutex_state);
			return data.state;
		}

		TCPConnectionState TCPConnection::getState()
		{
			MutexLock l(m_mutex_state);
			return m_state;
		}

		void TCPConnection::setState(TCPConnectionData &data, TCPConnectionState state)
		{
			MutexLock l(data.mutex_state);

			data.state = state;
			data.state_cond.signal(true);

			if (state == STATE_TRANSMITTING)
			{
				setState(STATE_TRANSMITTING);
			}
			else if (state == STATE_IDLE)
			{
				// only set global state to idle if both transfer states are idle
				if (data.outbound)
				{
					if (getState(m_incoming) == STATE_IDLE) setState(STATE_IDLE);
				}
				else
				{
					if (getState(m_outgoing) == STATE_IDLE) setState(STATE_IDLE);
				}
			}
		}

		void TCPConnection::setState(TCPConnectionState state)
		{
			MutexLock l(m_mutex_state);
			m_state = state;

			// Signal all waitForState that the state is changed
			m_cond_state.signal(true);
		}

		bool TCPConnection::waitForState(TCPConnectionData &data, TCPConnectionState state)
		{
			while (getState(data) != state)
			{
				data.state_cond.wait();

				if (getState(data) == STATE_CLOSED) return false;
			}

			return true;
		}

		bool TCPConnection::waitForState(TCPConnectionState state)
		{
			while (getState() != state)
			{
				m_cond_state.wait();

				if (getState() == STATE_CLOSED) return false;
			}

			return true;
		}

		void TCPConnection::idlechecking()
		{
			if (getState(m_incoming) == STATE_IDLE)
			{
				// TODO: do every second m_incoming.idletime++;
				//

				if (m_incoming.idletime > m_incoming.keepalive)
				{
					// TODO: disconnect
				}
			}

			if (getState(m_outgoing) == STATE_IDLE)
			{
				// TODO: send every x second a KEEP_ALIVE
			}
		}

		bool TCPConnection::datawaiting()
		{
			return ((getState(m_outgoing) == STATE_TRANSMITTING) ||
					(getState(m_outgoing) == STATE_HANDSHAKE));
		}

		void TCPConnection::transmit()
		{
			if ((getState(m_outgoing) == STATE_TRANSMITTING) ||
				(getState(m_outgoing) == STATE_HANDSHAKE))
			{
				MutexLock l(m_outgoing.mutex);

				// send only 1280 at once
				unsigned int data_seq = 1280;
				if (m_outgoing.remain < data_seq) data_seq = m_outgoing.remain;

				int sent_bytes = send(m_socket, m_outgoing.position, data_seq, 0);

				if (sent_bytes == -1)
				{
					disconnect();
					return;
				}

				m_outgoing.remain -= sent_bytes;
				m_outgoing.position += sent_bytes;

				if (m_outgoing.remain <= 0)
				{
					if (getState(m_outgoing) == STATE_HANDSHAKE)
					{
						// handshake sent, go to IDLE state
						setState(m_outgoing, STATE_IDLE);
					}
					else
					{
						// transfer successful
						setState(m_outgoing, STATE_TRANSFER_COMPLETE);
					}

					m_outgoing.data = NULL;
				}
			}
		}

		unsigned int TCPConnection::transmit(char *data, unsigned int size)
		{
			if (isClosed() || !waitForState(m_outgoing, STATE_IDLE))
			{
				return CONVERGENCE_LAYER_BUSY;
			}

			/**
			 * now cut the data into pieces of {chunksize}
			 * each one get a header in front
			 */

			unsigned int position = 0;
			bool isfirst = true;

			while (position < size)
			{
				unsigned int currentsize = (size - position);
				if (currentsize > m_chunksize ) currentsize = m_chunksize;

				// create a data message
				TCPMessage msg(currentsize, data);

				// mark the first block
				msg.setFirst(isfirst);
				isfirst = false;

				// adjust position
				position += currentsize;
				data += currentsize;

				// If there is no more data, this must be the last block
				if (position >= size) msg.setLast(true);

				MutexLock l(m_outgoing.mutex);

				m_outgoing.data = msg.getData();
				m_outgoing.position = msg.getData();
				m_outgoing.remain = msg.getSize();
				m_outgoing.length = msg.getSize();

				// trigger transfer
				setState(m_outgoing, STATE_TRANSMITTING);

				l.unlock();

				if ( !waitForState(m_outgoing, STATE_TRANSFER_COMPLETE) )
				{
					// connection closed - not all data transferred
					return position;
				}
			}

			// transfer complete - return to idle state
			setState(m_outgoing, STATE_IDLE);

			return size;
		}

		void TCPConnection::receive()
		{
			MutexLock l(m_incoming.mutex);

			unsigned char buffer[1280];
			size_t recv_size;
			int length = 0;
			u_int64_t stringsize = 0;

			// reset IDLE timer
			m_incoming.idletime = 0;

			switch (getState(m_incoming))
			{
				case STATE_INITIAL:
					// new connection, new data => awaiting a handshake
					setState(m_incoming, STATE_HANDSHAKE);
					setState(STATE_HANDSHAKE);
					break;

				case STATE_HANDSHAKE:
					/**
					 * receive the contact header
					 *
					 * magic = 4 byte
					 * version = 1 byte
					 * flags = 1 byte
					 * keepalive_interval = 2 byte
					 * eid length = SDNV
					 * eid = eid length
					 */

					// peek for incoming data
					recv_size = recv(m_socket, buffer, 1280, MSG_PEEK);

					// if the magic is received, check it first
					if (recv_size >= 4)
					{
						// check the magic
						if ( ( buffer[0] != 'd' ) ||
						     ( buffer[1] != 't' ) ||
						     ( buffer[2] != 'n' ) ||
						     ( buffer[3] != '!' ) )
						{
							// magic wrong -> disconnect
							disconnect();
							return;
						}
					}

					// check the version
					if (recv_size >= 5)
					{
						// check the magic
						if ( buffer[4] != BUNDLE_VERSION )
						{
							// version wrong -> disconnect
							disconnect();
							return;
						}
					}

					// the contact header has at least 10 bytes
					if (recv_size < 10) return;

					// try to read the size of the eid, begin at byte 9
					length = SDNV::decode(&buffer[8], 1280 - 8, &stringsize);

					// string size not transferred
					if (stringsize == 0) return;

					// full length is...?
					if ( recv_size >= (8 + length + stringsize) )
					{
						// full contactheader available, receive it
						recv(m_socket, buffer, (8 + length + stringsize), 0);

						// read the keep alive interval
						memcpy(&(m_incoming.keepalive), &buffer[6], 2);

						// read the eid in the contactheader
						char eid[stringsize + 1];
						memcpy(&eid[0], &buffer[8 + length], stringsize);
						eid[stringsize] = '\0';

						// store the eid
						m_eid = string(eid);

						// set the state to IDLE
						setState(m_incoming, STATE_IDLE);

						cout << "handshake received from " << m_eid << endl;
					}

					break;

				case STATE_IDLE:
					// incoming data, prepare for the next message
					setState(m_incoming, STATE_MESSAGE);
					break;

				case STATE_MESSAGE:
					/**
					 * read the header
					 *
					 * MSG_TYPE = 1/2 byte (TCPMessageHeaderType)
					 * start/end = 1/2 byte
					 * 		1 = end
					 * 		2 = start
					 * 		3 = start + end
					 * length = 1 byte
					 */

					// get the 2-byte-message-head
					if (recv(m_socket, &buffer[0], 1280, MSG_PEEK) >= 1)
					{
						switch (TCPMessageType(buffer[0] & 15))
						{
							case MSG_KEEPALIVE:
							{
								// clear the socket buffer
								recv(m_socket, &buffer[0], 1, 0);
								break;
							}

							case MSG_DATA_SEGMENT:
							{
								// check if there is a size (sdnv) available
								if (recv(m_socket, &buffer[0], 1280, MSG_PEEK) <= 1) break;

								u_int64_t chunksize = 0;
								size_t sdnv_len = SDNV::len(&buffer[1]);

								// if there is no sdnv, wait for another cycle
								if (sdnv_len <= 0) break;

								// read the buffer
								recv(m_socket, &buffer[0], sdnv_len + 1, 0);

								// read the size of the chunk
								SDNV::decode(&buffer[1], sdnv_len, &chunksize);

								// read flags
								bool isfirst = (buffer[0] & 64) != 0; // mask 00000010 = 64
								bool islast = (buffer[0] & 128) != 0; // mask 00000001 = 128

								if ( isfirst )
								{
									// first segment... create some space
									m_incoming.length = chunksize;
									m_incoming.data = (char*)calloc(m_incoming.length, sizeof(char));
									m_incoming.position = m_incoming.data;
									m_incoming.remain = m_incoming.length;
								}
								else
								{
									// next or last segment... expand the space
									m_incoming.data = (char*)realloc(m_incoming.data, m_incoming.length + chunksize);
									m_incoming.position = m_incoming.data;
									m_incoming.position += m_incoming.length;
									m_incoming.remain = chunksize;
									m_incoming.length += chunksize;
								}
								m_incoming.last = islast;

								setState(m_incoming, STATE_TRANSMITTING);
								break;
							}

							case MSG_SHUTDOWN:
							{
								// count the received bytes
								size_t bytes = recv(m_socket, &buffer[0], 1280, MSG_PEEK);
								u_int64_t reconnect = 0;
								size_t sdnv_len = 0;

								// read flags
								bool hasReason = (buffer[0] & 64) != 0; // mask 00000010 = 64
								bool hasReconnect = (buffer[0] & 128) != 0; // mask 00000001 = 128

								// check if the reason is available
								if (hasReason && bytes < 2) break;

								// check if the reconnect time is available
								if (hasReconnect && bytes < 3)
								{
									break;
								}
								else
								{
									sdnv_len = SDNV::len(&buffer[2]);

									if (sdnv_len > 0)
									{
										// read the size of the reconnect time
										SDNV::decode(&buffer[2], sdnv_len, &reconnect);
									}
									else
									{
										// if there is no sdnv, wait for another cycle
										break;
									}
								}

								// read the buffer
								recv(m_socket, &buffer[0], sdnv_len + 1 + hasReason, 0);

								// TODO: if IDLE or TRANSFER send a shutdown message
								// else just disconnect

								break;
							}

							case MSG_REFUSE_BUNDLE:
								// TODO: stop the current transfer
								break;

							case MSG_ACK_SEGMENT:
							{
								// check if there is a size (sdnv) available
								recv(m_socket, &buffer[0], 1280, MSG_PEEK);

								u_int64_t acksize = 0;
								size_t sdnv_len = SDNV::len(&buffer[1]);

								// if there is no sdnv, wait for another cycle
								if (sdnv_len <= 0) break;

								// read the buffer
								recv(m_socket, &buffer[0], sdnv_len + 1, 0);

								// read the size of the chunk
								SDNV::decode(&buffer[1], sdnv_len, &acksize);

								break;
							}
						}
					}

					break;

					case STATE_TRANSMITTING:
						if (m_incoming.remain < 1280)
						{
							// read bundle data
							recv_size = recv(m_socket, m_incoming.position, m_incoming.remain, 0);
						}
						else
						{
							// read bundle data
							recv_size = recv(m_socket, m_incoming.position, 1280, 0);
						}

						if (recv_size == 0)
						{
							disconnect();
							return;
						}

						m_incoming.position += recv_size;
						m_incoming.remain -= recv_size;

						if (m_incoming.remain <= 0)
						{
							if (m_incoming.last)
							{
								// if this is the last block then go in state COMPLETE
								setState(m_incoming, STATE_TRANSFER_COMPLETE);

								// transfer completed - merge the data parts to a bundle
								try {
									BundleFactory &fac = BundleFactory::getInstance();
									Bundle *b = fac.parse((unsigned char*)m_incoming.data, m_incoming.length);
									free(m_incoming.data);
									m_incoming.data = NULL;
									m_clayer.callbackBundleReceived(this, b);
								} catch (InvalidBundleData ex) {
									free(m_incoming.data);
									m_incoming.data = NULL;
								}
							}
							else
							{
								// if this is not the last block then go back to state MESSAGE
								setState(m_incoming, STATE_MESSAGE);
							}
						}
						break;

					case STATE_TRANSFER_COMPLETE:
						// wait for ack of all data
						setState(m_incoming, STATE_IDLE);
						break;

					case STATE_CLOSED:
						// do some cleanup
						if (m_incoming.data != NULL)
						{
							free(m_incoming.data);
							m_incoming.data = NULL;
						}

						// TODO: throw Exception?

					break;
			}
		}

		int TCPConnection::getSocket()
		{
			return m_socket;
		}

		void TCPConnection::disconnect()
		{
			// only closed the connection if open
			if (getState() == STATE_CLOSED) return;

			// set the state
			setState(STATE_CLOSED);

			// Debug
			if (m_eid == "")
			{
				cout << "connection to unknown closed" << endl;
			}
			else	cout << "connection to " << m_eid << " closed" << endl;

			// outgoing transfer complete?
			if (m_outgoing.data != NULL)
			{
				// no, set the state to closed
				setState(m_outgoing, STATE_CLOSED);
			}
//
//			// incoming transfer complete?
//			if (m_incoming.data != NULL)
//			{
//				// no, set the state to closed
//				setState(m_incoming, STATE_CLOSED);
//
//				// no, create a fragment out of the existing data
//				try {
//					// try to create a bundle out of the received data
//					Bundle *tmp = BundleFactory::createBundle((unsigned char*)m_incoming.data, m_incoming.length);
//
//					// clean up memory
//					free(m_incoming.data);
//					m_incoming.data = NULL;
//
//					// retrieved bytes
//					unsigned int retbytes = m_incoming.length - m_incoming.remain;
//
//					// get the size of the retrieved payload of the bundle
//					retbytes -= tmp->getOverhead();
//
//					// create a fragment
//					unsigned int offset = 0;
//					Bundle *fragment = BundleFactory::cut(tmp, retbytes, offset);
//
//					// cleanup the incomplete bundle
//					delete tmp;
//
//					// TODO: return the fragment to the bundle protocol
//					cout << "incoming fragment created" << fragment->toString() << endl;
//
//					delete fragment;
//
//				} catch (InvalidBundleData ex) {
//					free(m_incoming.data);
//					m_incoming.data = NULL;
//				}
//			}
		}

		bool TCPConnection::isClosed()
		{
			return getState() == STATE_CLOSED;
		}

		string TCPConnection::getRemoteURI()
		{
			if (getState() == STATE_INITIAL)
			{
				while (getState() < STATE_IDLE)
				{
					m_cond_state.wait();
				}

				if (getState() == STATE_CLOSED)
				{
					return "dtn:none";
				}
			}
			return m_eid;
		}

		void TCPConnection::handshake(unsigned char* data, unsigned int size)
		{
			MutexLock l(m_outgoing.mutex);

			if ( (getState() == STATE_INITIAL) || waitForState(m_outgoing, STATE_IDLE))
			{
				m_outgoing.data = (char*)data;
				m_outgoing.position = (char*)data;
				m_outgoing.remain = size;
				m_outgoing.length = size;

				// trigger transfer
				setState(m_outgoing, STATE_HANDSHAKE);
				setState(STATE_HANDSHAKE);
			}
		}
	}
}


