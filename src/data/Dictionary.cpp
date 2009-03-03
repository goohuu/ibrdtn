#include "data/Dictionary.h"
#include <iostream>
#include <cstdlib>
#include <cstring>


namespace dtn
{
namespace data
{

Dictionary::Dictionary()
{
}

Dictionary::~Dictionary()
{
}

/**
 * Fügt dem Dictionary einen Eintrag hinzu
 */
pair<unsigned int, unsigned int> Dictionary::add(string uri)
{
	unsigned int first_pos, second_pos;
	pair<string, string> uri_pair = split(uri);

	first_pos = get(uri_pair.first);

	if ( !exists(uri_pair.first) )
	{
		first_pos = getLength();
		m_entrys[first_pos] = uri_pair.first;
	}

	second_pos = get(uri_pair.second);

	if ( !exists(uri_pair.second) )
	{
		second_pos = getLength();
		m_entrys[second_pos] = uri_pair.second;
	}

	return make_pair(first_pos, second_pos);
}

bool Dictionary::exists(string entry)
{
	map<unsigned int, string, m_positionSort>::iterator iter = m_entrys.begin();

	while (iter != m_entrys.end())
	{
		if ( (*iter).second == entry )
		{
			return true;
		}

		iter++;
	}

	return false;
}

/**
 * Gibt eine URI zurück
 */
string Dictionary::get(unsigned int scheme, unsigned int ssp) const
{
	return get(scheme) + ":" + get(ssp);
}

/**
 * Gibt ein Entry zurück
 */
string Dictionary::get(unsigned int pos) const
{
	map<unsigned int, string, m_positionSort>::const_iterator iter = m_entrys.begin();

	while (m_entrys.end() != iter)
	{
		if ( (*iter).first == pos )
		{
			return (*iter).second;
		}
		iter++;
	}

	return "";
}

/**
 * Gibt die Position eines Entrys zurück
 */
unsigned int Dictionary::get(string entry) const
{
	map<unsigned int, string, m_positionSort>::const_iterator iter = m_entrys.begin();

	while (iter != m_entrys.end())
	{
		if ( (*iter).second == entry )
		{
			return (*iter).first;
		}

		iter++;
	}

	return -1;
}

/**
 * Ließt das Dictionary aus einem Datensatz aus
 */
void Dictionary::read(unsigned char *data, unsigned int length)
{
	char *dict_data = reinterpret_cast<char*>(data);

	// Alles löschen
	m_entrys.clear();

	string tmp;
	unsigned int position = 0;

	while (length > position)
	{
		tmp = dict_data;

		// Datensatz speichern
		m_entrys[position] = tmp;

		dict_data += tmp.length() + 1;
		position += tmp.length() + 1;
	}
}

/**
 * Gibt die Länge des Dictionary zurück
 */
unsigned int Dictionary::getLength() const
{
	if (m_entrys.size() == 0) return 0;

	map<unsigned int, string, m_positionSort>::const_iterator iter = m_entrys.end();
	iter--;

	return (*iter).first + (*iter).second.length() + 1;
}

unsigned int Dictionary::getSize() const
{
	return m_entrys.size();
}

/**
 * Schreibt das Dictionary in ein Datenarray und gibt die Anzahl der
 * verwendeten Zeichen zurück
 * @return Die Anzahl der verwendeten Zeichen
 */
unsigned int Dictionary::write(unsigned char *data) const
{
	unsigned int length = 0;
	string write_str;

	map<unsigned int, string, m_positionSort>::const_iterator iter = m_entrys.begin();

	while (iter != m_entrys.end())
	{
		write_str = (*iter).second;

		// Schreibe den String
		memcpy(data, write_str.data(), write_str.length());
		data += write_str.length();
		length += write_str.length();

		// Schreibe den Stringtrenner
		data[0] = '\0';
		data++;
		length++;

		// Iterator ein nach vorn
		iter++;
	}


	return length;
}

/* Trennt eine URI in Scheme und SSP
 *
 * "result" muss ein Array mit 2 Einträgen sein
 */
pair<string, string> Dictionary::split(string uri) throw (exceptions::InvalidDataException)
{
	pair<string, string> result;

	size_t splitter = uri.find(":", 0);

	// Raus wenn kein Splitter gefunden wurde
	if (splitter == string::npos)
	{
		throw exceptions::InvalidDataException(uri + " ist keine gültige URI");
	}

	result.first = uri.substr(0, splitter);
	result.second = uri.substr(splitter + 1, uri.length() - splitter);

	return result;
}

}
}
