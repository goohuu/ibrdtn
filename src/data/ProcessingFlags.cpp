#include "data/ProcessingFlags.h"


namespace dtn
{
	namespace data
	{
		ProcessingFlags::ProcessingFlags() : m_value(0)
		{

		}

		ProcessingFlags::ProcessingFlags(unsigned int value) : m_value(value)
		{

		}

		ProcessingFlags::~ProcessingFlags()
		{

		}

		bool ProcessingFlags::getFlag(unsigned int flag)
		{
			unsigned int bit = 1 << flag;

			return (( m_value & bit ) == bit);
		}

		void ProcessingFlags::setFlag(unsigned int flag, bool value)
		{
			if (value)
			{
				m_value |= 1 << flag;
			}
			else
			{
				m_value &= ~(1 << flag);
			}
		}

		unsigned int ProcessingFlags::getValue()
		{
			return m_value;
		}
	}
}
