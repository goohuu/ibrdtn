#ifndef PROCESSINGFLAGS_H_
#define PROCESSINGFLAGS_H_

namespace dtn
{
	namespace data
	{
		class ProcessingFlags
		{
			public:
				/**
				 * constructor
				 */
				ProcessingFlags();

				/**
				 * constructor with a default value
				 * @param value The initial value of the flags.
				 */
				ProcessingFlags(unsigned int value);

				/**
				 * destructor
				 */
				~ProcessingFlags();

				/**
				 * Set a specific flag.
				 * @param flag The number of the flag (0...n)
				 * @param value true, if the flag should be set.
				 */
				void setFlag(unsigned int flag, bool value);

				/**
				 * Get the value of a specific flag.
				 * @param flag The number of the flag (0...n)
				 * @return true, if the flag is set.
				 */
				bool getFlag(unsigned int flag);

				/**
				 * Get a value contains all the flags.
				 * @return A numeric value with all flags.
				 */
				unsigned int getValue();

			private:
				unsigned int m_value;
		};
	}
}

#endif /*PROCESSINGFLAGS_H_*/
