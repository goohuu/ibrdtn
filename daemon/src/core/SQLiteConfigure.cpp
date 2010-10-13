/*
 * SQLiteConfigure.cpp
 *
 *  Created on: 30.03.2010
 *      Author: Myrtus
 */
#include "core/SQLiteConfigure.h"
#include <sqlite3.h>
#include <iostream>
#include "ibrcommon/thread/MutexLock.h"

namespace dtn{
namespace core{
	ibrcommon::Mutex SQLiteConfigure::_mutex;
	bool SQLiteConfigure::_isSet;

	void SQLiteConfigure::configure(){
		//Configure SQLite Library
		ibrcommon::MutexLock lock = ibrcommon::MutexLock(_mutex);
		if(_isSet = false){
//			int err = sqlite3_config(SQLITE_CONFIG_SINGLETHREAD);
			int err = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
//			int err = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
			if(err!=SQLITE_OK){
				std::cerr << "SQLITEConfigure fehlgeschlagen " << err <<std::endl;
			}
		}
	}
}
}
