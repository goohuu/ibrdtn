/*
 * Bundle.h
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#ifndef API_BUNDLE_H_
#define API_BUNDLE_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrcommon/data/BLOB.h"

#include <iostream>
#include <fstream>

namespace dtn
{
	namespace api
	{
		/**
		 * definition for a friend-class
		 */
		class Client;

		/**
		 * This bundle object should be used by the applications to create, send and receive bundles.
		 */
		class Bundle
		{
			friend class Client;

		public:
			/**
			 * Define the Bundle Security Mode
			 * SEC_OFF no security actions will be taken
			 * SEC_SIGNED the bundle will be signed before leaving
			 * SEC_ENCRYPTED the bundle will be encrypted before leaving
			 * SEC_ENC_SIGNED the bundle will be singed and encrypted before leaving
			 */
			enum BUNDLE_SECURITY
			{
				SEC_OFF = 0,
				SEC_SIGNED = 1,
				SEC_ENCRYPTED = 2,
				SEC_ENC_SIGNED = 3
			};

			/**
			 * Define the Bundle Priorities
			 * PRIO_LOW low priority for this bundle
			 * PRIO_MEDIUM medium priority for this bundle
			 * PRIO_HIGH high priority for this bundle
			 */
			enum BUNDLE_PRIORITY
			{
				PRIO_LOW = 0,
				PRIO_MEDIUM = 1,
				PRIO_HIGH = 2
			};

			/**
			 * Constructor for a bundle object without a destination
			 */
			Bundle();

			/**
			 * Constructor for a bundle object
			 * @param destination defines the destination of this bundle.
			 */
			Bundle(dtn::data::EID destination);

			/**
			 * Destructor
			 */
			virtual ~Bundle();

			/**
			 * Set the security parameters for this bundle.
			 */
			void setSecurity(BUNDLE_SECURITY s);

			/**
			 * Returns the security parameters for this bundle.
			 */
			BUNDLE_SECURITY getSecurity();

                        /**
                         * Set the lifetime of a bundle
                         */
                        void setLifetime(unsigned int lifetime);

                        /**
                         * Returns the lifetime of a bundle
                         */
                        unsigned int getLifetime();

			/**
			 * Set the priority for this bundle.
			 */
			void setPriority(BUNDLE_PRIORITY p);

			/**
			 * Returns the priority for this bundle.
			 */
			BUNDLE_PRIORITY getPriority();

			/**
			 * Serialize this object into a stream.
			 */
			virtual void write(std::ostream &stream);
			virtual void read(std::istream &stream);

			/**
			 * Serialize a Bundle into a stream.
			 */
			friend std::ostream &operator<<(std::ostream &stream, dtn::api::Bundle &b)
			{
				b.write(stream);
				return stream;
			}

			friend std::istream &operator>>(std::istream &stream, dtn::api::Bundle &b)
			{
				b.read(stream);
				return stream;
			}

			/**
			 * Returns a reference to the data block of this bundle.
			 */
			ibrcommon::BLOB::Reference getData();

			dtn::data::EID getDestination();
			dtn::data::EID getSource();

		protected:
			/**
			 * Constructor for a bundle object.
			 * @param b include a bundle of the data library in this bundle.
			 */
			Bundle(dtn::data::Bundle &b);

			dtn::data::Bundle _b;
			BUNDLE_SECURITY _security;
			BUNDLE_PRIORITY _priority;
                        unsigned int _lifetime;
		};
	}
}

#endif /* API_BUNDLE_H_ */
