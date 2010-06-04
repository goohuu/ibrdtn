#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include "ibrcommon/Exceptions.h"

#include <stdexcept>
#include <string>


using namespace std;

namespace dtn
{
		/**
		 * If some data is invalid, this exception is thrown.
		 */
		class InvalidDataException : public ibrcommon::Exception
		{
			public:
				InvalidDataException(string what = "Invalid input data.") throw() : Exception(what)
				{
				};
		};

		class InvalidProtocolException : public dtn::InvalidDataException
		{
		public:
			InvalidProtocolException(string what = "The received data does not match the protocol.") throw() : dtn::InvalidDataException(what)
			{
			};
		};

		class SerializationFailedException : public dtn::InvalidDataException
		{
		public:
			SerializationFailedException(string what = "The serialization failed.") throw() : dtn::InvalidDataException(what)
			{
			};
		};

		class MissingObjectException : public ibrcommon::Exception
		{
		public:
			MissingObjectException(string what = "Object not available.") throw() : Exception(what)
			{
			};
		};
}

#endif /*EXCEPTIONS_H_*/
