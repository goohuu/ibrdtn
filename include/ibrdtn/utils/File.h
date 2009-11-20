/*
 * File.h
 *
 *  Created on: 20.11.2009
 *      Author: morgenro
 */

#ifndef FILE_H_
#define FILE_H_

#include "ibrdtn/default.h"
#include <iostream>
#include <map>
#include <vector>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace dtn
{
	namespace utils
	{
		class File
		{
		public:
			File(const string path);
			virtual ~File();
			unsigned char getType() const;
			int getFiles(list<File> &files);

			bool isSystem();
			bool isDirectory();
			string getPath() const;
			int remove(bool recursive = false);

		private:
			File(const string path, const unsigned char t);
			const string _path;
			unsigned char _type;
		};
	}
}

#endif /* FILE_H_ */
