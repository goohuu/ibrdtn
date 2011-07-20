/*
 * SecurityCertificateManager.cpp
 *
 *  Created on: Apr 2, 2011
 *      Author: roettger
 */

#include "SecurityCertificateManager.h"
#include "Configuration.h"

#include <cstdlib>

#include <ibrcommon/Logger.h>

namespace dtn {

namespace security {

	SecurityCertificateManager::SecurityCertificateManager()
		: _initialized(false), _cert(NULL), _privateKey(NULL)
	{
	}

	SecurityCertificateManager::~SecurityCertificateManager() {
	}

	bool SecurityCertificateManager::isInitialized()
	{
		return _initialized;
	}

//	void SecurityCertificateManager::addChainCertificate(ibrcommon::File & certificate)
//    {
//    }

    X509 *SecurityCertificateManager::getCert()
    {
        return _cert;
    }

    EVP_PKEY *SecurityCertificateManager::getPrivateKey()
    {
        return _privateKey;
    }

    ibrcommon::File SecurityCertificateManager::getTrustedCAPath() const
    {
        return _trustedCAPath;
    }

    void
    SecurityCertificateManager::initialize()
    {
    	ibrcommon::File certificate = dtn::daemon::Configuration::getInstance().getSecurity().getCA();
    	ibrcommon::File privateKey = dtn::daemon::Configuration::getInstance().getSecurity().getKey();
    	ibrcommon::File trustedCAPath = dtn::daemon::Configuration::getInstance().getSecurity().getTrustedCAPath();

    	FILE *fp = NULL;
		X509 *cert = NULL;
		EVP_PKEY *key = NULL;

		ibrcommon::MutexLock l(_initialization_lock);
		if(_initialized){
			IBRCOMMON_LOGGER(info) << "SecurityCertificateManager: already initialized." << IBRCOMMON_LOGGER_ENDL;
			return;
		}

		/* read the certificate */
		fp = fopen(certificate.getPath().c_str(), "r");
		if(!fp || !PEM_read_X509(fp, &cert, NULL, NULL)){
			IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: Could not read the certificate File: " << certificate.getPath() << "." << IBRCOMMON_LOGGER_ENDL;
			if(fp){
				fclose(fp);
			}
			throw ibrcommon::IOException("Could not read the certificate.");
		}
		fclose(fp);

		/* read the private key */
		fp = fopen(privateKey.getPath().c_str(), "r");
		if(!fp || !PEM_read_PrivateKey(fp, &key, NULL, NULL)){
			IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: Could not read the PrivateKey File: " << privateKey.getPath() << "." << IBRCOMMON_LOGGER_ENDL;
			if(fp){
				fclose(fp);
			}
			throw ibrcommon::IOException("Could not read the PrivateKey.");
		}
		fclose(fp);

		/* check trustedCAPath */
		if(!trustedCAPath.isDirectory()){
			IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: the trustedCAPath is not valid: " << trustedCAPath.getPath() << "." << IBRCOMMON_LOGGER_ENDL;
			throw ibrcommon::IOException("Invalid trustedCAPath.");
		}

		_cert = cert;
		_privateKey = key;
		_trustedCAPath = trustedCAPath;
		_initialized = true;

		IBRCOMMON_LOGGER(info) << "SecurityCertificateManager: Initialization succeeded." << IBRCOMMON_LOGGER_ENDL;
    }

	void
	SecurityCertificateManager::startup()
	{
		if(_initialized){
			CertificateManagerInitEvent::raise(_cert, _privateKey, _trustedCAPath);
		}
	}

	void
	SecurityCertificateManager::terminate()
	{
		/* nothing to do */
	}

	const std::string
	SecurityCertificateManager::getName() const
	{
		return "SecurityCertificateManager";
	}

	bool
	SecurityCertificateManager::validateSubject(X509 *certificate, const dtn::data::EID &eid)
	{
		if(!certificate || eid.getString().empty()){
			return false;
		}

		X509_NAME *cert_name;
		X509_NAME_ENTRY *name_entry;
		ASN1_STRING *eid_string;
		int lastpos = -1;
		int entry_count;
		unsigned char *utf8_eid;
		int utf8_eid_len;

		/* retrieve the X509_NAME structure */
		if(!(cert_name = X509_get_subject_name(certificate))){
			return false;
		}

		/* convert the eid to an ASN1_STRING, it is needed later for comparison. */
		eid_string = ASN1_STRING_type_new(V_ASN1_PRINTABLESTRING);
		if(!eid_string){
			IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: Error while creating an ASN1_STRING." << IBRCOMMON_LOGGER_ENDL;
			return false;
		}
		/* TODO this function returns an int, but the return value is undocumented */
		/* the -1 indicates, that strlen() should be used to calculate the data length */
		ASN1_STRING_set(eid_string, eid.getString().c_str(), -1);

		utf8_eid_len = ASN1_STRING_to_UTF8(&utf8_eid, eid_string);
		if(utf8_eid_len <= 0){
			IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: ASN1_STRING_to_UTF8() returned " << utf8_eid_len << "." << IBRCOMMON_LOGGER_ENDL;
			return false;
		}

		/* process all entries that are of type NID_commonName */
		while(true){
			lastpos = X509_NAME_get_index_by_NID(cert_name, NID_commonName, lastpos);
			if(lastpos == -1){
				break;
			}

			/* get the NAME_ENTRY structure */
			name_entry = X509_NAME_get_entry(cert_name, lastpos);
			if(!name_entry){
				IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: X509_NAME_get_entry returned NULL unexpectedly." << IBRCOMMON_LOGGER_ENDL;
				continue;
			}

			/* retrieve the string */
			ASN1_STRING *asn1 = X509_NAME_ENTRY_get_data(name_entry);
			if(!asn1){
				IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: X509_NAME_ENTRY_get_data returned NULL unexpectedly." << IBRCOMMON_LOGGER_ENDL;
				continue;
			}

			unsigned char *utf8_cert_name;
			int utf8_cert_len;
			utf8_cert_len = ASN1_STRING_to_UTF8(&utf8_cert_name, asn1);
			if(utf8_cert_len <= 0){
				IBRCOMMON_LOGGER(error) << "SecurityCertificateManager: ASN1_STRING_to_UTF8() returned " << utf8_cert_len << "." << IBRCOMMON_LOGGER_ENDL;
				continue;
			}

			/* check if the string fits the given EID */
			if(utf8_cert_len != utf8_eid_len){
				continue;
			}
			if(memcmp(utf8_eid, utf8_cert_name, utf8_eid_len) == 0){
				OPENSSL_free(utf8_cert_name);
				OPENSSL_free(utf8_eid);
				return true;
			}
			OPENSSL_free(utf8_cert_name);
		}

		OPENSSL_free(utf8_eid);

		char *subject_line = X509_NAME_oneline(cert_name, NULL, 0);
		if(subject_line){
			IBRCOMMON_LOGGER(warning) << "SecurityCertificateManager: Certificate does not fit. EID: " << eid.getString() << ", Certificate Subject: " << subject_line << "." << IBRCOMMON_LOGGER_ENDL;
			delete subject_line;
		}
		return false;
	}

}

}
