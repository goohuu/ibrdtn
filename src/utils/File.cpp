/*
 * File.cpp
 *
 *  Created on: 20.11.2009
 *      Author: morgenro
 */

#include "ibrdtn/utils/File.h"

namespace dtn
{
	namespace utils
	{
		File::File(const string path, const unsigned char t)
		 : _path(path), _type(t)
		{
		}

		File::File(const string path)
		 : _path(path), _type(DT_UNKNOWN)
		{
			struct stat s;
			int type;

			stat(path	.c_str(), &s);

			type = s.st_mode & S_IFMT;

			switch (type)
			{
				case S_IFREG:
					_type = DT_REG;
					break;

				case S_IFLNK:
					_type = DT_LNK;
					break;

				case S_IFDIR:
					_type = DT_DIR;
					break;

				default:
					_type = DT_UNKNOWN;
					break;
			}
		}

		File::~File()
		{}

		unsigned char File::getType() const
		{
			return _type;
		}

		int File::getFiles(list<File> &files)
		{
			if (!isDirectory()) return -1;

			DIR *dp;
			struct dirent *dirp;
			if((dp = opendir(_path.c_str())) == NULL) {
				return errno;
			}

			while ((dirp = readdir(dp)) != NULL)
			{
				string name = string(dirp->d_name);
				stringstream ss; ss << getPath() << "/" << name;
				File file(ss.str(), dirp->d_type);
				files.push_back(file);
			}
			closedir(dp);
		}

		bool File::isSystem()
		{
			if ((_path.substr(_path.length() - 2, 2) == "..") || (_path.substr(_path.length() - 1, 1) == ".")) return true;
			return false;
		}

		bool File::isDirectory()
		{
			if (_type == DT_DIR) return true;
			return false;
		}

		string File::getPath() const
		{
			return _path;
		}

		int File::remove(bool recursive)
		{
			int ret;

			if (isSystem()) return -1;

			if (isDirectory())
			{
				if (recursive)
				{
					// container for all files
					list<File> files;

					// get all files in this directory
					if ((ret = getFiles(files)) < 0)
						return ret;

					for (list<File>::iterator iter = files.begin(); iter != files.end(); iter++)
					{
						if (!(*iter).isSystem())
						{
							if ((ret = (*iter).remove(recursive)) < 0)
								return ret;
						}
					}
				}

				//rmdir(getPath());
				cout << "remove directory: " << getPath() << endl;
			}
			else
			{
				//remove(getPath());
				cout << "remove file: " << getPath() << endl;
			}

			return 0;
		}
	}
}
