#ifndef LOWPANCONNECTION_H_
#define LOWPANCONNECTION_H_

#include "net/LOWPANConvergenceLayer.h"

#include <ibrcommon/thread/Queue.h>
#include <ibrcommon/net/lowpanstream.h>

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class lowpanstream;
		class LOWPANConnectionSender : public ibrcommon::JoinableThread
		{
			public:
				LOWPANConnectionSender(lowpanstream &_stream);
				virtual ~LOWPANConnectionSender();

				/**
				 * Queueing a job, originally coming from the DTN core, to work its way down to the CL through the lowpanstream
				 * @param job Reference to the job conatining EID and bundle
				 */
				void queue(const ConvergenceLayer::Job &job);
				void run();
				bool __cancellation();

			private:
				lowpanstream &_stream;
				ibrcommon::Queue<ConvergenceLayer::Job> _queue;
		};

		class LOWPANConvergenceLayer;
		class LOWPANConnection : public ibrcommon::DetachedThread
		{
		public:
			/**
			 * LOWPANConnection constructor
			 * @param _address IEEE 802.15.4 short address to identfy the connection
			 * @param LOWPANConvergenceLayer reference
			 */
			LOWPANConnection(unsigned short _address, LOWPANConvergenceLayer &cl);

			virtual ~LOWPANConnection();

			/**
			 * IEEE 802.15.4 short address of the node this connection handles data for.
			 */
			unsigned short _address;

			/**
			 * Getting the lowpanstream connected with the LOWPANConnection
			 * @return lowpanstream reference
			 */
			lowpanstream& getStream();

			void run();
			void setup();
			void finally();

			/**
			 * Instance of the LOWPANConnectionSender
			 */
			LOWPANConnectionSender _sender;

		private:
			bool _running;
			lowpanstream _stream;
			LOWPANConvergenceLayer &_cl;
		};
	}
}
#endif /*LOWPANCONNECTION_H_*/
