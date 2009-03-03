#ifndef PRIMARYFLAGS_H_
#define PRIMARYFLAGS_H_

#include "ProcessingFlags.h"

namespace dtn
{
namespace data
{

/**
 * Bestimmt eine Priorität
 */
enum PriorityFlag
{
	BULK = 0,
	NORMAL = 1,
	EXPEDITED = 2,
	RESERVED = 3
};	

enum PrimaryProcBits
{
	FRAGMENT = 0,
	APPDATA_IS_ADMRECORD = 1,
	DONT_FRAGMENT = 2,
	CUSTODY_REQUESTED = 3,
	DESTINATION_IS_SINGLETON = 4,
	ACKOFAPP_REQUESTED = 5,
	RESERVED_6 = 6,
	PRIORITY_BIT1 = 7,
	PRIORITY_BIT2 = 8,
	CLASSOFSERVICE_9 = 9,
	CLASSOFSERVICE_10 = 10,
	CLASSOFSERVICE_11 = 11,
	CLASSOFSERVICE_12 = 12,
	CLASSOFSERVICE_13 = 13,
	REQUEST_REPORT_OF_BUNDLE_RECEPTION = 14,
	REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE = 15,
	REQUEST_REPORT_OF_BUNDLE_FORWARDING = 16,
	REQUEST_REPORT_OF_BUNDLE_DELIVERY = 17,
	REQUEST_REPORT_OF_BUNDLE_DELETION = 18,
	STATUS_REPORT_REQUEST_19 = 19,
	STATUS_REPORT_REQUEST_20 = 20
};
	

/**
 * Ermöglicht das setzen und lesen von Parametern die Bitweise in einem Integer
 * codiert sind.
 */
class PrimaryFlags : public data::ProcessingFlags
{
public:
	/**
	 * Defaultkonstruktor für die ProcFlags
	 */
	PrimaryFlags();
	
	/**
	 * Konstruktor für die ProcFlags
	 * @param value Wertvorgabe um Parameter eines Bündels zu lesen
	 */
	PrimaryFlags(unsigned int value);
	
	/**
	 * Desktrutor
	 */
	virtual ~PrimaryFlags();
	
	/**
	 * Bestimmt ob der Fragment Flag gesetzt ist
	 * @return Gibt True zurück wenn das Fragment Flag gesetzt ist
	 */
	bool isFragment();
	
	/**
	 * Setzt den Fragment Flag
	 * @param value True, wenn das Fragment Flag gesetzt werden soll
	 */
	void setFragment(bool value);
	
	/**
	 * Bestimmt ob das Bundle einen administrativen Record enthält
	 * @return True, wenn das Bundle einen administrativen Record enthält
	 */
	bool isAdmRecord();
	
	/**
	 * Setzt ob das Bundle einen administrativen Record enthält
	 * @param value True, wenn der Flag für den administrativen Record gesetzt werden soll 
	 */
	void setAdmRecord(bool value);
	
	/**
	 * Bestimmt ob eine Fragmentierung des Bundles verboten ist
	 * @return True, wenn die Fragmentierung des Bundles verboten ist
	 */
	bool isFragmentationForbidden();
	
	/**
	 * Setzt ob eine Fragmentierung des Bundles verboten ist
	 * @param value True, wenn eine Fragmentierung des Bundles verboten ist
	 */
	void setFragmentationForbidden(bool value);
	
	/**
	 * Bestimmt ob Custody für ein Bundle angefordert wird
	 * @return True, wenn Custody angefordert wird
	 */
	bool isCustodyRequested();
	
	/**
	 * Setzt ob Custody für ein Bundle angefordert wird
	 * @param value True, wenn Custody für ein Bundle angefordert werden soll
	 */
	void setCustodyRequested(bool value);
	
	/**
	 * Bestimmt ob die Destination EID ein Singleton ist
	 * @return True, wenn die Destination EID ein Singleton ist, sonst False
	 */
	bool isEIDSingleton();
	
	/**
	 * Setzt ob die Destination EID ein Singleton ist
	 * @param value True, wenn die Destination EID ein Singleton ist, sonst False
	 */
	void setEIDSingleton(bool value);
	
	/**
	 * Bestimmt ob eine Bestätigung der Applikationsschicht angefordert wird
	 * @return True, wenn eine Bestätigung der Applikationsschicht angefordert wird
	 */
	bool isAckOfAppRequested();
	
	/**
	 * Setzt ob eine Bestätigung der Applikationsschicht angefordert werden soll
	 * @param value True, wenn eine Bestätigung der Applikationsschicht angefordert werden soll
	 */
	void setAckOfAppRequested(bool value);
	
	/**
	 * Bestimmt die Priorität die ein Bundle hat
	 * @return Einen Wert des Enum PriorityFlag um eine Priorität festzulesen
	 */
	PriorityFlag getPriority();
	
	/**
	 * Setzt die Priorität eines Bundles
	 * @param value Einen Wert des Enum PriorityFlag um eine Priorität festzulesen
	 */
	void setPriority(PriorityFlag value);

};

}
}

#endif /*PRIMARYFLAGS_H_*/
