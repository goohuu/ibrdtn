#ifndef CUSTODYSIGNALBLOCK_H_
#define CUSTODYSIGNALBLOCK_H_

#include "AdministrativeBlock.h"
#include "Exceptions.h"

namespace dtn
{
namespace data
{

enum CUSTODY_FIELDS
{
	CUSTODY_ADMFIELD = 0,
	CUSTODY_STATUS = 1,
	CUSTODY_FRAGMENT_OFFSET = 2,
	CUSTODY_FRAGMENT_LENGTH = 3,
	CUSTODY_TIMEOFSIGNAL = 4,
	CUSTODY_BUNDLE_TIMESTAMP = 5,
	//CUSTODY_UNKNOWN = 6,
	CUSTODY_BUNDLE_SEQUENCE = 6,
	CUSTODY_BUNDLE_SOURCE_LENGTH = 7,
	CUSTODY_BUNDLE_SOURCE = 8
};

/**
 * Ein DiscoverBlock kann an den Primärblock angehängt werden und wird
 * verwendet um sich selbst gegenüber anderen DTN Knoten bemerkbar zu machen.
 */
class CustodySignalBlock : public data::AdministrativeBlock
{
public:
	/**
	 * Konstruktor
	 */
	CustodySignalBlock(Block *block);
	CustodySignalBlock(NetworkFrame *frame);

	/**
	 * destructor
	 */
	virtual ~CustodySignalBlock();

	bool forFragment() const;

	unsigned int getFragmentOffset() const;
	void setFragmentOffset(unsigned int value);

	unsigned int getFragmentLength() const;
	void setFragmentLength(unsigned int value);

	unsigned int getTimeOfSignal() const;
	void setTimeOfSignal(unsigned int value);

	unsigned int getCreationTimestamp() const;
	void setCreationTimestamp(unsigned int value);

	unsigned int getCreationTimestampSequence() const;
	void setCreationTimestampSequence(unsigned int value);

	string getSource() const;
	void setSource(string value);

	bool isAccepted() const;
	void setAccepted(bool value);

	bool match(const Bundle &b) const;
	void setMatch(const Bundle &b);
private:
	unsigned int getField(CUSTODY_FIELDS field) const;
};

}
}

#endif /*CUSTODYSIGNALBLOCK_H_*/
