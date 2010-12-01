/*
 * SecurityManager.h
 *
 *  Created on: 27.07.2009
 *      Author: morgenro
 */

#ifndef SECURITYMANAGER_H_
#define SECURITYMANAGER_H_

#include "ibrdtn/api/Bundle.h"

namespace dtn
{
	namespace api
	{
		class SecurityManager
		{
		public:
			virtual ~SecurityManager();

			/**
			 * Set the keys for encryption and signing.
			 */
			static void initialize(const std::string &privKey, const std::string &pubKey, const std::string &ca);

			/**
			 * Set the default security for new bundles.
			 */
			static void setDefault(const Bundle::BUNDLE_SECURITY s);

			/**
			 * Get the default security for new bundles.
			 */
			static Bundle::BUNDLE_SECURITY getDefault();

			/**
			 * Check the security blocks of a bundle
			 */
			static bool validate(const dtn::api::Bundle &b);

		private:
			/**
			 * no instantiate
			 */
			SecurityManager();

			/**
			 * no copy
			 */
			SecurityManager(SecurityManager&) {};

			static Bundle::BUNDLE_SECURITY _default;
		};
	}
}

#endif /* SECURITYMANAGER_H_ */
