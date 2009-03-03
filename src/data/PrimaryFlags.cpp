#include "data/PrimaryFlags.h"

namespace dtn
{
namespace data
{
	PrimaryFlags::PrimaryFlags() : ProcessingFlags()
	{
		setPriority(BULK);
	}
	
	PrimaryFlags::PrimaryFlags(unsigned int value) : ProcessingFlags(value)
	{
		setPriority(BULK);
	}
	
	PrimaryFlags::~PrimaryFlags()
	{
	}	
	
	bool PrimaryFlags::isFragment()
	{
		return getFlag(FRAGMENT);
	}
	
	void PrimaryFlags::setFragment(bool value)
	{
		setFlag(FRAGMENT, value);
	}
	
	bool PrimaryFlags::isAdmRecord()
	{
		return getFlag(APPDATA_IS_ADMRECORD);
	}
	
	void PrimaryFlags::setAdmRecord(bool value)
	{
		setFlag(APPDATA_IS_ADMRECORD, value);
	}
	
	bool PrimaryFlags::isFragmentationForbidden()
	{
		return getFlag(DONT_FRAGMENT);	
	}
	
	void PrimaryFlags::setFragmentationForbidden(bool value)
	{
		setFlag(DONT_FRAGMENT, value);
	}
	
	bool PrimaryFlags::isCustodyRequested()
	{
		return getFlag(CUSTODY_REQUESTED);
	}
	
	void PrimaryFlags::setCustodyRequested(bool value)
	{
		setFlag(CUSTODY_REQUESTED, value);
	}
	
	bool PrimaryFlags::isEIDSingleton()
	{
		return getFlag(DESTINATION_IS_SINGLETON);
	}
	
	void PrimaryFlags::setEIDSingleton(bool value)
	{
		setFlag(DESTINATION_IS_SINGLETON, value);
	}
	
	bool PrimaryFlags::isAckOfAppRequested()
	{
		return getFlag(ACKOFAPP_REQUESTED);
	}
	
	void PrimaryFlags::setAckOfAppRequested(bool value)
	{
		setFlag(ACKOFAPP_REQUESTED, value);
	}
	
	PriorityFlag PrimaryFlags::getPriority()
	{
		if ( getFlag(PRIORITY_BIT1) )
		{
			if ( getFlag(PRIORITY_BIT2) ) 	return RESERVED;
			else				return NORMAL;
		}
		else
		{
			if ( getFlag(PRIORITY_BIT2) ) 	return EXPEDITED;
			else				return BULK;
		}
	}
	
	void PrimaryFlags::setPriority(PriorityFlag value)
	{
		switch (value)
		{
			case BULK:
				setFlag(PRIORITY_BIT1, false);
				setFlag(PRIORITY_BIT2, false);					
			break;
			
			case NORMAL:
				setFlag(PRIORITY_BIT1, true);
				setFlag(PRIORITY_BIT2, false);				
			break;
			
			case EXPEDITED:
				setFlag(PRIORITY_BIT1, false);
				setFlag(PRIORITY_BIT2, true);				
			break;
			
			case RESERVED:
				setFlag(PRIORITY_BIT1, true);
				setFlag(PRIORITY_BIT2, true);
			break;
		}
	}
}
}
