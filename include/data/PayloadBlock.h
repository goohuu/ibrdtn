#ifndef PAYLOADBLOCK_H_
#define PAYLOADBLOCK_H_

#include <stdio.h>

#include "Block.h"
#include "BlockFlags.h"
#include "Exceptions.h"

namespace dtn
{
namespace data
{

/**
 * Ein Payload Block kann an den Primärblock angehängt werden und enthält
 * Nutzdaten für die Applikationsschicht
 */
class PayloadBlock : public data::Block
{
public:
	static const unsigned char BLOCK_TYPE = 1;

	PayloadBlock(NetworkFrame *frame);
	PayloadBlock(Block *block);

	/**
	 * destructor
	 */
	virtual ~PayloadBlock();

	/**
	 * Gibt einen Zeiger auf den Payload zurück
	 * @return Einen Zeiger auf das Payload Datenarray
	 */
	unsigned char* getPayload() const;

	/**
	 * Set the payload of this PayloadBlock. It copy the given data to the existing data array of the bundle.
	 */
	void setPayload(const unsigned char *data, unsigned int size);

	/**
	 * Get the range of the payload in the data array.
	 * e.g. if the payload data begins at 15th byte and has a size of 64 byte, then the pair <15, 79> is returned.
	 * @return a pair of two unsigned integer
	 */
	pair<unsigned int, unsigned int> getPayloadRange() const;

	/**
	 * Gibt die Länge des Payloads zurück
	 * @return Die Länge als Integer
	 */
	unsigned int getLength() const;
};
}
}

#endif /*PAYLOADBLOCK_H_*/
