/** DTN C API */

#include "ibrdtn/api/dtn_api.h"
#include "ibrcommon/net/tcpclient.h"
#include "ibrdtn/api/Client.h"
#include "ibrdtn/api/StringBundle.h"
#include <string.h>
#include <stdarg.h>
#include <unistd.h>



class CAPIGateway : public dtn::api::Client
{
	public:
		CAPIGateway(string app, void (*process_bundle)(const void * data, uint32_t size), void (*status_callback)(struct dtn_notification info) = NULL , string address = "127.0.0.1", int port = 4550)
		: dtn::api::Client(app, _tcpclient), _tcpclient(address, port)
		{
			cout << "Connecting daemon...";
			this->connect(); //Rückgabewerte?!!?
			cout << "Done" << endl;
			this->process_bundle=process_bundle;
			this->status_callback=status_callback;

			tx_buffer_position=0;
			tx_buffer_size=KBYTE(64);
			tx_buffer=(char *)malloc(tx_buffer_size);
			dst_eid=NULL;
			if (tx_buffer == NULL) {
				cout << "C_API: Error allocating tx_buffer." << endl;
				tx_buffer_size=0;
			}
			if (this->process_bundle == NULL) { //enter synchronous mode
				if ( pipe(sync_pipe) != 0) {
					perror("C_API: Error creating pipe for synchronous operation");
				}
			}
		};

		/**
		 * Destructor of the connection.
		 */
		virtual ~CAPIGateway()
		{
			// Close the tcp connection.
			_tcpclient.close();
		};

		void send(char *dst_uri, char *data, uint32_t length) {
			dtn::data::Bundle b;
			b._destination = dtn::data::EID(dst_uri);
			dtn::data::PayloadBlock &payload = b.push_back<dtn::data::PayloadBlock>();

			// add the data
			(*payload.getBLOB()).write(data, length);

			// transmit the packet
			dtn::data::DefaultSerializer(*this) << b;
			Client::flush();
		}

		void dtn_write(const void *data, uint32_t length) {
			uintptr_t offset=0;
			uint32_t tocopy=0;
			while (offset < length) {
				if ((tx_buffer_size-tx_buffer_position) >= (length-offset))
					tocopy=(length-offset);
				else
					tocopy=tx_buffer_size-tx_buffer_position;
				//cout << "CPY " << tocopy << " because size is " << tx_buffer_size << " and position is " << tx_buffer_position << endl;
				memcpy(tx_buffer+tx_buffer_position,(uint8_t *)data+offset,tocopy);
				offset+=tocopy;
				tx_buffer_position+=tocopy;
				if (tx_buffer_position == tx_buffer_size) { //TX buffer full, transmit
					cout << "TX !" << endl;
					transmit_buffer();
				}
			}
		}

		int32_t dtn_read(void *buf, uint32_t length) {
			if (process_bundle != NULL) {
				cout << "C_API: Can't use dtn_read() for endpoint in asynchronous mode" << endl;
				return -1;
			}
			return ::read(sync_pipe[0],buf, length);
		}

		void transmit_buffer() {
			if (dst_eid==NULL) {
				cout << "C_API: transmit request without destination: Clearing buffer " << endl;
				tx_buffer_position=0;
				return;
			}
			if (tx_buffer_position == 0) //is empty
				return;
			dtn::data::Bundle b;
			b._destination = dtn::data::EID(this->dst_eid);
			dtn::data::PayloadBlock &payload = b.push_back<dtn::data::PayloadBlock>();

			// add the data
			(*payload.getBLOB()).write(tx_buffer, tx_buffer_position); //+1 is wrong =?!?

			// transmit the packet
			dtn::data::DefaultSerializer(*this) << b;
			Client::flush();
			tx_buffer_position=0;
		}

		void set_dsteid(const char *dst) {
			if (this->dst_eid != NULL) {
				free(this->dst_eid);
			}
			this->dst_eid = (char *)malloc(strlen(dst)+1);
			if (this->dst_eid == NULL) {
				cout << "C_API: Set destination eid failed. No memory" << endl;
				return;
			}
			strcpy(this->dst_eid,dst);
		}

		void set_txchunksize(uint32_t chunksize) {
			transmit_buffer(); //flush old buffer
			if (tx_buffer != NULL)
				free(tx_buffer);
			tx_buffer=(char *)malloc(chunksize);
			if (tx_buffer == NULL) {
				cout << "C_API: Set chunksize failed. No memory" << endl;
				tx_buffer_size=0;
				return;
			}
			cout << "C_API: New tx_buffer_size: " << chunksize << endl;
			tx_buffer_size=chunksize;
		}

	private:
		void (*process_bundle)(const void * data, uint32_t size);
		void (*status_callback)(struct dtn_notification info);
		void *recvd;
		ibrcommon::Mutex recv_mutex;
		int sync_pipe[2];

		char *tx_buffer;
		uint32_t tx_buffer_size;
		uint32_t tx_buffer_position;
		char *dst_eid;

		void notify(uint16_t status, uint32_t data) {
			struct dtn_notification n;
			if (status_callback != NULL) {
				n.status=status;
				n.data=data;
				recv_mutex.enter();
				status_callback(n);
				recv_mutex.leave();
			}
		}

		ibrcommon::tcpclient _tcpclient;



		/**
		 * In this API bundles are received asynchronous. To receive bundles it is necessary
		 * to overload the Client::received()-method. This will be call on a incoming bundles
		 * by another thread.
		 */
		void received(dtn::api::Bundle &b)
		{
			if (process_bundle != NULL) {
				receive_async(b);
			}
			else {
				receive_sync(b);
			}
		}

		void receive_async(dtn::api::Bundle &b) {
			int32_t len=b.getData().getSize();
			//cout << "You probably wouldn't believe it but I received " << len << " bytes stuff " << endl;
			if (len < 0 || len > 65536 ) {  //todo: set tx_buffer size
				cout << "Ignoring bundle" << endl;
				notify(DTN_NOTIFY_TOOBIG,len);
				return;
			}
			recvd=malloc(len);
			(*b.getData()).read((char *)recvd, len);
			recv_mutex.enter();
			process_bundle(recvd,len);
			recv_mutex.leave();
			free(recvd);
		}

		void receive_sync(dtn::api::Bundle &b) {
			uint32_t chunksize=KBYTE(64);
			char *buffer=(char *)malloc(chunksize);
			uint32_t len=b.getData().getSize();
			uint32_t offset=0;
			if (buffer == NULL) {
				cout << "C_API: receive_sync(): Can't allocate buffer " << endl;
				return;
			}
			//cout << "You probably wouldn't believe it but I received " << len << " bytes stuff " << endl;
			while(len != 0) {
				if (len <= chunksize) {
					//cout << "Write " << len << endl;
					(*b.getData()).read(buffer, len);
					::write(sync_pipe[1],buffer,len);
					break;
				}
				else {
					//cout << "Write " << chunksize << endl;
					(*b.getData()).read(buffer, chunksize);
					offset+=chunksize; len-=chunksize;
					::write(sync_pipe[1],buffer,chunksize);
				}
			}
		}
};


struct dtn_fd {
	CAPIGateway *gate;
};

//TODO alloc with[MAX_DTN_FDS], init on DT dameon init
static struct dtn_fd dtn_fds[] = {{NULL},{NULL},{NULL},{NULL}};

extern "C" int32_t dtn_register_endpoint(char *ep, void (*process_bundle)(const void * data, uint32_t size), void (*status_callback)(struct dtn_notification info)) {
	int i;
	cout << "Register Endpoint " << ep << endl;
	for (i=0; i<MAX_DTN_FDS; i++) {
		if (dtn_fds[i].gate == NULL) {
			cout << "Alloc dtn_fd " << i << endl;
			dtn_fds[i].gate = new CAPIGateway(ep, process_bundle);

			return i;
		}
	}
	cout << "C_API: DTN fds exhausted " << endl;
	return -1;
}


extern "C" void dtn_close_endpoint(DTN_EP ep) {
	if (ep < 0 || ep >= MAX_DTN_FDS) {
		cout << "C_API: Invalid ep descriptor in close(): " << ep << endl;
		return;
	}
	if (dtn_fds[ep].gate == NULL) {
		cout << "C_API: Trying to close non existing ep " << ep << endl;
		return;
	}
	dtn_fds[ep].gate->close();
	dtn_fds[ep].gate==NULL;

}

extern "C" void dtn_write(DTN_EP ep, const void *data, uint32_t length) {
	if (ep < 0 || ep >= MAX_DTN_FDS) {
		cout << "C_API: Invalid ep descriptor in dtn_write(): " << ep << endl;
		return;
	}
	if (dtn_fds[ep].gate == NULL) {
		cout << "C_API: Trying to dtn_write() to non existing ep " << ep << endl;
		return;
	}
	dtn_fds[ep].gate->dtn_write(data,length);
}

extern "C" int32_t dtn_read(DTN_EP ep, void *buf, uint32_t length) {
	if (ep < 0 || ep >= MAX_DTN_FDS) {
		cout << "C_API: Invalid ep descriptor in dtn_read(): " << ep << endl;
		return -1;
	}
	if (dtn_fds[ep].gate == NULL) {
		cout << "C_API: Trying to dtn_read() from non existing ep " << ep << endl;
		return -1;
	}
	return dtn_fds[ep].gate->dtn_read(buf,length);
}

extern "C" void dtn_endpoint_set_option(DTN_EP ep, uint16_t option, ...) {
	va_list ap;
	va_start(ap,option);
	if (ep < 0 || ep >= MAX_DTN_FDS) {
		cout << "C_API: Invalid ep descriptor in dtn_endpoint_set_option(): " << ep << endl;
		return;
	}
	if (dtn_fds[ep].gate == NULL) {
		cout << "C_API: Trying to dtn_endpoint_set_option() to non existing ep " << ep << endl;
		return;
	}
	switch (option) {
		case DTN_OPTION_DSTEID:
			dtn_fds[ep].gate->set_dsteid(va_arg(ap, char *));
			break;
		case DTN_OPTION_TXCHUNKSIZE:
			dtn_fds[ep].gate->set_txchunksize(va_arg(ap, uint32_t));
			break;
		case DTN_OPTION_FLUSH:
			dtn_fds[ep].gate->transmit_buffer();
			break;
		default:
			cout << "C_API: Unkown option " << option << " in dtn_endpoint_set_option()" << endl;
	}
}

extern "C" void dtn_send_bundle(int32_t ep , char *dst_uri, char *data, uint32_t length) {
	if (ep < 0 || ep >= MAX_DTN_FDS || dtn_fds[ep].gate == NULL) {
		cout << "C_API: Invalid ep descriptor in sendBundle(): " << ep << endl;
		return;
	}
	dtn_fds[ep].gate->send(dst_uri, data, length);
}

extern "C" void dtn_hithere() {
	cout << "Muahahaharrr" << endl;
}

