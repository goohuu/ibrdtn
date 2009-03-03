#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include <string>
#include "Exceptions.h"
#include <map>

using namespace std;

namespace dtn
{
namespace data
{

/**
 * Diese Klasse ermöglicht, dass erstellen eines Dictionary für den Primärblock
 */
class Dictionary
{
public:
	/**
	 * Konstruktor
	 */
	Dictionary();

	/**
	 * Destruktor
	 */
	virtual ~Dictionary();

	/**
	 * Fügt dem Dictionary einen Eintrag hinzu
	 */
	pair<unsigned int, unsigned int> add(string uri);

	/**
	 * Gibt eine URI zurück
	 */
	string get(unsigned int scheme, unsigned int ssp) const;

	/**
	 * Gibt ein Entry zurück
	 */
	string get(unsigned int pos) const;

	/**
	 * Gibt die Position eines Entrys zurück
	 */
	unsigned int get(string entry) const;

	/**
	 * Ließt das Dictionary aus einem Datensatz aus
	 */
	void read(unsigned char *data, unsigned int length);

	/**
	 * Gibt die Länge des Dictionary zurück
	 */
	unsigned int getLength() const;

	/**
	 * Gibt die Anzahl der Einträge zurück
	 */
	unsigned int getSize() const;

	/**
	 * Schreibt das Dictionary in ein Datenarray und gibt die Anzahl der
	 * verwendeten Zeichen zurück
	 * @return Die Anzahl der verwendeten Zeichen
	 */
	unsigned int write(unsigned char *data) const;

	/**
	 * Split a URI into Scheme and SSP
	 * @param uri The URI to split.
	 * @return A pair of Scheme and SSP.
	 */
	static pair<string, string> split(string uri) throw (exceptions::InvalidDataException);

private:

	/**
	 * Prüft ob ein bestimmter Eintrag bereits enthalten ist
	 */
	bool exists(string entry);

	/**
	 * Sortierfunktion für die Map
	 */
	struct m_positionSort {
		bool operator()( unsigned int pos1, unsigned int pos2 ) const {
			return (pos1 < pos2);
		}
	};

	map<unsigned int, string, m_positionSort> m_entrys;
};

}
}

#endif /*DICTIONARY_H_*/
