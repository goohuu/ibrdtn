/*
 * appstreambuf.h
 *
 *  Created on: 20.11.2009
 *      Author: morgenro
 */

#ifndef APPSTREAMBUF_H_
#define APPSTREAMBUF_H_

#include "ibrdtn/default.h"
#include <streambuf>
#include <vector>
#include <string>
#include <stdio.h>

namespace dtn
{
	namespace utils
	{
		class appstreambuf : public std::streambuf
		{
		public:
			enum { BUF_SIZE = 512 };

			enum Mode
			{
				MODE_READ = 0,
				MODE_WRITE = 1
			};

			appstreambuf(std::string command, appstreambuf::Mode mode);
			virtual ~appstreambuf();

		protected:
			virtual int underflow();
			virtual int sync();
			virtual std::char_traits<char>::int_type overflow( std::char_traits<char>::int_type m = traits_type::eof() );

		private:
			std::vector< char_type > m_buf;
			FILE *_handle;
		};
	}
}

#endif /* APPSTREAMBUF_H_ */
