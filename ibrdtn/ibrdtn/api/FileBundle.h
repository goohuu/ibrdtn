/*
 * FileBundle.h
 *
 *  Created on: 24.07.2009
 *      Author: morgenro
 */

#ifndef FILEBUNDLE_H_
#define FILEBUNDLE_H_

#include "ibrdtn/api/Bundle.h"
#include <iostream>

namespace dtn
{
	namespace api
	{
		/**
		 * This class could be used to send whole files through the
		 * bundle protocol.
		 */
		class FileBundle : public dtn::api::Bundle
		{
		public:
			/**
			 * Constructor need a destination and a file stream.
			 */
			//FileBundle(dtn::data::EID destination, std::fstream &file);

			FileBundle(dtn::data::EID destination, string filename);

			/**
			 * Destruktor.
			 */
			virtual ~FileBundle();

		private:
			//std::fstream &_file;
			std::string _filename;
		};
	}
}

#endif /* FILEBUNDLE_H_ */
