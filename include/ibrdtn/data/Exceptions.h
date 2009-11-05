#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include "ibrdtn/data/Bundle.h"

#include <stdexcept>
#include <string>


using namespace std;

namespace dtn
{
	namespace exceptions
	{
		/**
		 * Base class for all exceptions in the bundle protocol
		 */
		class Exception : public exception
		{
			public:
				/**
				 * default constructor
				 */
				Exception() throw()
				{};

				Exception(const exception&) throw()
				{};

				virtual ~Exception() throw()
				{};

				/**
				 * Get the explaining reason as string value.
				 * @return The reason as string value.
				 */
				virtual const char* what() const throw()
				{
					return m_what.c_str();
				}

				/**
				 * constructor with attached string value as reason.
				 * @param what The detailed reason for this exception.
				 */
				Exception(string what) throw()
				{
					m_what = what;
				};

			private:
				string m_what;
		};

		/**
		 * This is thrown if a method isn't implemented.
		 */
		class NotImplementedException : public Exception
		{
		public:
			NotImplementedException(string what = "This method isn't implemented.") throw() : Exception(what)
			{
			};
		};

		/**
		 * This is thrown if Input/Output error happens.
		 */
		class IOException : public Exception
		{
		public:
			IOException(string what = "Input/Output error.") throw() : Exception(what)
			{
			};
		};

		/**
		 * If the use of a given field is invalid for this method, this exception is thrown.
		 */
		class InvalidFieldException : public Exception
		{
			public:
				InvalidFieldException(string what = "The given field is invalid.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If a given field don't exists, this exception is thrown.
		 */
		class FieldDoesNotExist : public Exception
		{
			public:
				FieldDoesNotExist(string what = "This field don't exists.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If the parsed data of a bundle is invalid, this exception is thrown.
		 */
		class InvalidBundleData : public Exception
		{
			public:
				InvalidBundleData(string what = "The bundle contains invalid data.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If the decoding of a SDNV failed, this exception is thrown.
		 */
		class SDNVDecodeFailed : public Exception
		{
			public:
				SDNVDecodeFailed(string what = "Decoding of a SDNV failed.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If a choosed option is invalid, this exception is thrown.
		 */
		class InvalidOptionException : public Exception
		{
			public:
				InvalidOptionException(string what = "Invalid option.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If some data is invalid, this exception is thrown.
		 */
		class InvalidDataException : public Exception
		{
			public:
				InvalidDataException(string what = "Invalid input data.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If no route can be found, this exception is thrown.
		 */
		class NoRouteFoundException : public Exception
		{
			public:
				NoRouteFoundException(string what = "No route found.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If no bundle can be found, this exception is thrown.
		 */
		class NoBundleFoundException : public Exception
		{
			public:
				NoBundleFoundException(string what = "No bundle available.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If a given size is to small for a method, this exception is thrown.
		 */
		class MaxSizeTooSmallException : public Exception
		{
			public:
				MaxSizeTooSmallException(string what = "Maximum size is too small.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If a error occurs during a fragmentation process, this exception is thrown.
		 */
		class FragmentationException : public Exception
		{
			public:
				FragmentationException(string what = "De-/fragmentation not possible.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If a fragmentation is needed, but the do-not-fragment Bit is set, this exception is thrown.
		 */
		class DoNotFragmentBitSetException : public FragmentationException
		{
			public:
				DoNotFragmentBitSetException(string what = "Do not fragment-bit is set. Fragmentation not possible.") throw() : FragmentationException(what)
				{
				};
		};

//		/**
//		 * If a the parsed data of a bundle isn't complete, this exception is thrown.
//		 * Optional: A fragment of the available part is returned with this exception.
//		 */
//		class BundleFragmentedException : public FragmentationException, InvalidBundleData
//		{
//			public:
//				BundleFragmentedException(data::Bundle fragment, string what = "Bundle data is fragmented.") throw()
//				 : FragmentationException(what), InvalidBundleData(what), m_fragment(fragment)
//				{
//				};
//
//				data::Bundle getFragment() throw()
//				{
//					return m_fragment;
//				}
//
//			private:
//				data::Bundle m_fragment;
//		};

		/**
		 * If a bundle expires, this exception is thrown.
		 */
		class BundleExpiredException : public Exception
		{
			public:
				BundleExpiredException(string what = "This bundle has expired.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If no more space is available, e.g. for storing a bundle, this exception is thrown.
		 */
		class NoSpaceLeftException : public Exception
		{
			public:
				NoSpaceLeftException(string what = "No space left.") throw() : Exception(what)
				{
				};
		};

		/**
		 * If no neighbour was found, this exception is thrown.
		 */
		class NoNeighbourFoundException : public Exception
		{
			public:
				NoNeighbourFoundException(string what = "No neighbour was found.") throw() : Exception(what)
				{
				};
		};

		class NoTimerFoundException : public Exception
		{
		public:
			NoTimerFoundException(string what = "No matching timer was found.") throw() : Exception(what)
			{
			};
		};

		class MissingObjectException : public Exception
		{
		public:
			MissingObjectException(string what = "Object not available.") throw() : Exception(what)
			{
			};
		};

		class PermissionDeniedException : public Exception
		{
		public:
			PermissionDeniedException(string what = "Permission denied.") throw() : Exception(what)
			{
			};
		};

		class DuplicateBundleException : public Exception
		{
		public:
			DuplicateBundleException(string what = "Duplicate found.") throw() : Exception(what)
			{
			};
		};
	}
}

#endif /*EXCEPTIONS_H_*/
