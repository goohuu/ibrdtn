/*
 * SecurityKeyManager.h
 *
 *  Created on: 02.12.2010
 *      Author: morgenro
 */

#ifndef SECURITYKEYMANAGER_H_
#define SECURITYKEYMANAGER_H_

#include <ibrdtn/data/EID.h>

namespace dtn
{
	namespace security
	{
		class SecurityKeyManager
		{
		public:
			virtual ~SecurityKeyManager() {};
			virtual void prefetchKey(const dtn::data::EID &ref) = 0;
			virtual std::string getKey(const dtn::data::EID &ref) = 0;
		};
	}
}

#endif /* SECURITYKEYMANAGER_H_ */
