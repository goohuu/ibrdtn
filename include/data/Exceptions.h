#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <string>
using namespace std;

namespace dtn
{
	namespace data
	{
		/**
		 * Forward declaration
		 */
		class Bundle;
	}

	namespace exceptions
	{
		/**
		 * Base class for all exceptions in the bundle protocol
		 */
		class Exception
		{
			public:
				/**
				 * default constructor
				 */
				Exception()
				{};

				/**
				 * constructor with attached string value as reason.
				 * @param what The detailed reason for this exception.
				 */
				Exception(string what)
				{
					m_what = what;
				};

				/**
				 * Get the explaining reason as string value.
				 * @return The reason as string value.
				 */
				string what()
				{
					return m_what;
				};

			private:
				string m_what;
		};

		/**
		 * If the use of a given field is invalid for this method, this exception is thrown.
		 */
		class InvalidFieldException : public Exception
		{
			public:
				InvalidFieldException() : Exception("The given field is invalid.")
				{
				};

				InvalidFieldException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If a given field don't exists, this exception is thrown.
		 */
		class FieldDoesNotExist : public Exception
		{
			public:
				FieldDoesNotExist() : Exception("This field don't exists.")
				{
				};

				FieldDoesNotExist(string what) : Exception(what)
				{
				};
		};

		/**
		 * If the parsed data of a bundle is invalid, this exception is thrown.
		 */
		class InvalidBundleData : public Exception
		{
			public:
				InvalidBundleData() : Exception("The bundle contains invalid data.")
				{
				};

				InvalidBundleData(string what) : Exception(what)
				{
				};
		};

		/**
		 * If the decoding of a SDNV failed, this exception is thrown.
		 */
		class SDNVDecodeFailed : public Exception
		{
			public:
				SDNVDecodeFailed() : Exception("Decoding of a SDNV failed.")
				{
				};

				SDNVDecodeFailed(string what) : Exception(what)
				{
				};
		};

		/**
		 * If a choosed option is invalid, this exception is thrown.
		 */
		class InvalidOptionException : public Exception
		{
			public:
				InvalidOptionException() : Exception("Invalid option.")
				{
				};

				InvalidOptionException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If some data is invalid, this exception is thrown.
		 */
		class InvalidDataException : public Exception
		{
			public:
				InvalidDataException() : Exception("Invalid input data.")
				{
				};

				InvalidDataException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If no schedule can be found, this exception is thrown.
		 */
		class NoScheduleFoundException : public Exception
		{
			public:
				NoScheduleFoundException() : Exception("No schedule found.")
				{
				};

				NoScheduleFoundException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If no bundle can be found, this exception is thrown.
		 */
		class NoBundleFoundException : public Exception
		{
			public:
				NoBundleFoundException() : Exception("No bundle available.")
				{
				};

				NoBundleFoundException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If a given size is to small for a method, this exception is thrown.
		 */
		class MaxSizeTooSmallException : public Exception
		{
			public:
				MaxSizeTooSmallException() : Exception("Maximum size is too small.")
				{
				};

				MaxSizeTooSmallException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If a error occurs during a fragmentation process, this exception is thrown.
		 */
		class FragmentationException : public Exception
		{
			public:
				FragmentationException() : Exception("De-/fragmentation not possible.")
				{
				};

				FragmentationException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If a fragmentation is needed, but the do-not-fragment Bit is set, this exception is thrown.
		 */
		class DoNotFragmentBitSetException : public FragmentationException
		{
			public:
				DoNotFragmentBitSetException() : FragmentationException("Do not fragment-bit is set. Fragmentation not possible.")
				{
				};

				DoNotFragmentBitSetException(string what) : FragmentationException(what)
				{
				};
		};

		/**
		 * If a the parsed data of a bundle isn't complete, this exception is thrown.
		 * Optional: A fragment of the available part is returned with this exception.
		 */
		class BundleFragmentedException: public FragmentationException, InvalidBundleData
		{
			public:
				BundleFragmentedException(data::Bundle *fragment) : FragmentationException("Bundle data is fragmented."), InvalidBundleData("Bundle data is fragmented."), m_fragment(fragment)
				{
				};

				BundleFragmentedException(string what, data::Bundle *fragment) : FragmentationException(what), InvalidBundleData(what), m_fragment(fragment)
				{
				};

				data::Bundle *getFragment()
				{
					return m_fragment;
				}

			private:
				data::Bundle *m_fragment;
		};

		/**
		 * If a bundle expires, this exception is thrown.
		 */
		class BundleExpiredException : public Exception
		{
			public:
				BundleExpiredException() : Exception("This bundle has expired.")
				{
				};

				BundleExpiredException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If no more space is available, e.g. for storing a bundle, this exception is thrown.
		 */
		class NoSpaceLeftException : public Exception
		{
			public:
				NoSpaceLeftException() : Exception("No space left.")
				{
				};

				NoSpaceLeftException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If no neighbour was found, this exception is thrown.
		 */
		class NoNeighbourFoundException : public Exception
		{
			public:
				NoNeighbourFoundException() : Exception("No neighbour was found.")
				{
				};

				NoNeighbourFoundException(string what) : Exception(what)
				{
				};
		};

		/**
		 * If a transfer couldn't completed, this exception is thrown.
		 * Optional: A fragment of the not transferred part is returned with this exception.
		 */
		class TransferNotCompletedException: public Exception
		{
			public:
				TransferNotCompletedException(data::Bundle *fragment) : Exception("Transfer not completed."), m_fragment(fragment)
				{
				};

				TransferNotCompletedException(string what, data::Bundle *fragment) : Exception(what), m_fragment(fragment)
				{
				};

				data::Bundle *getFragment()
				{
					return m_fragment;
				}

			private:
				data::Bundle *m_fragment;
		};
	}
}

#endif /*EXCEPTIONS_H_*/
