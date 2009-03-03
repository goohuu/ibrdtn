#include "data/NetworkFrame.h"
#include <iostream>
#include "data/SDNV.h"
#include <cstdlib>
#include <cstring>
#include <stdexcept>


namespace dtn
{
namespace data
{
	NetworkFrame::NetworkFrame() : m_size(0)
	{
		// Speicherbereich freiräumen
		m_data = (unsigned char*)calloc(0, sizeof(unsigned char));
	}

	NetworkFrame::NetworkFrame(const unsigned char* data, const unsigned int size)
	 : m_data(NULL), m_size(size)
	{
		m_data = (unsigned char*)calloc(size, sizeof(unsigned char));

		// Mache eine Kopie der Daten
		memcpy(m_data, data, size);
	}

	NetworkFrame::NetworkFrame(unsigned char* data, unsigned int size, map<unsigned int, unsigned int> fieldsizes)
	 : m_data(data), m_fieldsizes(fieldsizes), m_size(size)
	{}

	NetworkFrame::~NetworkFrame()
	{
		free(m_data);
	}

	NetworkFrame::NetworkFrame(const NetworkFrame& k)
	 : m_data(NULL), m_size(k.m_size)
	{
		m_data = (unsigned char*)calloc(k.m_size, sizeof(unsigned char));
		memcpy(m_data, k.m_data, k.m_size);
		m_fieldsizes = k.m_fieldsizes;
	}

	NetworkFrame& NetworkFrame::operator=(const NetworkFrame &k)
	{
		if (this != &k)
		{
			free(m_data);
			m_data = (unsigned char*)calloc(k.m_size, sizeof(unsigned char));
			memcpy(m_data, k.m_data, k.m_size);
			m_fieldsizes = k.m_fieldsizes;
		}

		return *this;
	}

	void NetworkFrame::setFieldSizeMap(map<unsigned int, unsigned int> mapping)
	{
		m_fieldsizes = mapping;
		updateSize();
	}

	void NetworkFrame::updateSize()
	{
		// recalc m_size
		m_size = 0;

		map<unsigned int, unsigned int>::const_iterator iter = m_fieldsizes.begin();

		while (m_fieldsizes.end() != iter)
		{
			m_size += (*iter).second;
			iter++;
		}
	}

	map<unsigned int, unsigned int>& NetworkFrame::getFieldSizeMap()
	{
		return m_fieldsizes;
	}

	unsigned char* NetworkFrame::getData() const
	{
		return m_data;
	}

	unsigned int NetworkFrame::getSize() const
	{
		return m_size;
	}

	unsigned int NetworkFrame::getSize(const unsigned int field) const
	{
		try {
			return m_fieldsizes.at(field);
		} catch (std::out_of_range ex) {
			return 0;
		}
	}

	unsigned int NetworkFrame::getPosition(const unsigned int field) const
	{
		// Anzahl der Felder
		unsigned int fieldcount = m_fieldsizes.size();

		// Errechnete Feldposition
		unsigned int position = 0;

		for (unsigned int i = 0; i < fieldcount; i++)
		{
			if (field == i)
			{
				break;
			}
			else
			{
				position += getSize(i);
			}
		}

		return position;
	}

	unsigned int NetworkFrame::append(const u_int64_t value)
	{
		// Anzahl der Felder = neue ID
		unsigned int fieldcount = m_fieldsizes.size();

		// Codierungslänge ermitteln
		unsigned int size = data::SDNV::encoding_len(value);

		// Framedaten um neues Feld erweitern
		m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * (m_size + size) );

		// Kopiere den Datenzeiger
		unsigned char *framedata = m_data;

		// Zeiger auf das Ende vom alten Frame setzen
		framedata += m_size;

		// Daten in das Feld kopieren
		data::SDNV::encode(value, framedata, size);

		// Neue Gesamtgröße merken
		m_size += size;

		// Feldgröße merken
		m_fieldsizes[fieldcount] = size;

		// Feld-ID zurückgeben
		return fieldcount;
	}

	unsigned int NetworkFrame::append(int value)
	{
		return append( (u_int64_t) value );
	}

	unsigned int NetworkFrame::append(unsigned int value)
	{
		return append( (u_int64_t) value );
	}

	unsigned int NetworkFrame::append(double value)
	{
		return append( reinterpret_cast<unsigned char*>(&value), sizeof(double) );
	}

	unsigned int NetworkFrame::append(const unsigned char* data, unsigned int size)
	{
		// Anzahl der Felder = neue ID
		unsigned int fieldcount = m_fieldsizes.size();

		// Framedaten um neues Feld erweitern
		m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * (m_size + size) );

		// Kopiere den Datenzeiger
		unsigned char *framedata = m_data;

		// Zeiger auf das Ende vom alten Frame setzen
		framedata += m_size;

		// Daten in das Feld kopieren
		memcpy(framedata, data, size);

		// Neue Gesamtgröße merken
		m_size += size;

		// Feldgröße merken
		m_fieldsizes[fieldcount] = size;

		// Feld-ID zurückgeben
		return fieldcount;
	}

	unsigned int NetworkFrame::append(const char value)
	{
		return append((const unsigned char)value);
	}

	unsigned int NetworkFrame::append(const string value)
	{
		return append((unsigned char*)value.c_str(), value.length());
	}

	unsigned int NetworkFrame::append(const unsigned char value)
	{
		// Anzahl der Felder = neue ID
		unsigned int fieldcount = m_fieldsizes.size();

		unsigned int size = sizeof(unsigned char);

		// Framedaten um neues Feld erweitern
		m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * (m_size + size) );

		// Kopiere den Datenzeiger
		unsigned char *framedata = m_data;

		// Zeiger auf das Ende vom alten Frame setzen
		framedata += m_size;

		// Daten in das Feld kopieren
		framedata[0] = value;

		// Neue Gesamtgröße merken
		m_size += size;

		// Feldgröße merken
		m_fieldsizes[fieldcount] = size;

		// Feld-ID zurückgeben
		return fieldcount;
	}

	unsigned char* NetworkFrame::get(const unsigned int field) const
	{
		// Kopiere den Datenzeiger
		unsigned char* data = m_data;

		// Setze den Zeiger auf die Position
		data += getPosition(field);

		// Gebe die Feldlänge zurück
		return data;
	}

	u_int64_t NetworkFrame::getSDNV(const unsigned int field) const
	{
		// Rückgabewert
		u_int64_t ret_val = 0;

		// Datenzeigers mit der richtigen Position
		const unsigned char *data = get(field);

		// Daten decodieren
		data::SDNV::decode(data, data::SDNV::len(data), &ret_val);

		// Ergebnis zurückgeben
		return ret_val;
	}

	double NetworkFrame::getDouble(const unsigned int field) const
	{
		unsigned char *data = get(field);
		double *ret = reinterpret_cast<double*>(data);

		return *ret;
	}

	unsigned char NetworkFrame::getChar(const unsigned int field) const
	{
		// Datenzeigers mit der richtigen Position
		const unsigned char *data = get(field);

		return data[0];
	}

	string NetworkFrame::getString(const unsigned int field) const
	{
		// Hole Zeiger auf das Datenfeld
		unsigned char *data = get(field);
		unsigned int length = getSize(field);

		// Erstelle einen Puffer für den String
		char *str_data = (char*)calloc(length + 1, sizeof(char));

		// Kopiere Stringdaten
		memcpy(str_data, data, length);

		// Füge abschließendes Symbol ein
		str_data[length] = '\0';

		// In einen String umwandeln
		string ret = str_data;

		// Gebe den Puffer frei
		free(str_data);

		// Gebe den String zurück
		return ret;
	}

	void NetworkFrame::set(const unsigned int field, const u_int64_t value)
	{
		// Codierungslänge ermitteln
		unsigned int size = data::SDNV::encoding_len(value);

		// Passe das Datenfeld an
		changeSize( field, size );

		// Kopiere den Datenzeiger
		unsigned char *framedata = m_data;

		// Setze den Zeiger auf die Position
		framedata += getPosition(field);

		// Daten in das Feld kopieren
		data::SDNV::encode(value, framedata, size);
	}

	void NetworkFrame::set(const unsigned int field, const int value)
	{
		set(field, (u_int64_t) value);
	}

	void NetworkFrame::set(const unsigned int field, const unsigned int value)
	{
		set(field, (u_int64_t) value);
	}

	void NetworkFrame::set(const unsigned int field, const char value)
	{
		set(field, (unsigned char) value);
	}

	void NetworkFrame::set(const unsigned int field, const unsigned char value)
	{
		// Passe das Datenfeld an
		changeSize( field, sizeof(unsigned char) );

		// Kopiere den Datenzeiger
		unsigned char *framedata = m_data;

		// Setze den Zeiger auf die Position
		framedata += getPosition(field);

		// Schreibe nun die Daten
		framedata[0] = value;
	}

	void NetworkFrame::set(const unsigned int field, const string value)
	{
		set(field, (unsigned char*)value.c_str(), value.length());
	}

	void NetworkFrame::set(const unsigned int field, double value)
	{
		set(field, reinterpret_cast<unsigned char*>(&value), sizeof(double));
	}

	void NetworkFrame::set(const unsigned int field, const unsigned char* data, const unsigned int size)
	{
		// Passe das Datenfeld an
		changeSize( field, size );

		// Kopiere den Datenzeiger
		unsigned char *framedata = m_data;

		// Setze den Zeiger auf die Position
		framedata += getPosition(field);

		// Schreibe nun die Daten
		memcpy(framedata, data, size);
	}

	void NetworkFrame::remove(const unsigned int field)
	{
		// change the field size to zero
		changeSize(field, 0);

		// get the count of the available fields
		unsigned int endfield = m_fieldsizes.size() - 1;

		// get through all fields after {field}
		for (unsigned int i = field; i < endfield; i++)
		{
			// copy length of the next field
			m_fieldsizes[i] = m_fieldsizes[i + 1];
		}

		// remove the last element
		m_fieldsizes.erase(endfield);
	}

	void NetworkFrame::insert(const unsigned int field)
	{
		// get the count of the available fields
		unsigned int endfield = m_fieldsizes.size();

		// get through all fields after {field}
		for (unsigned int i = endfield; i > field; i--)
		{
			// copy length of the next field
			m_fieldsizes[i] = m_fieldsizes[i - 1];
		}

		// add a empty field
		m_fieldsizes[field] = 0;
	}

	void NetworkFrame::changeSize(unsigned int field, unsigned int size)
	{
		// Kopiere den Datenzeiger
		unsigned char *framedata = m_data;

		// Setze den Zeiger auf die Position
		framedata += getPosition(field);

		// Wenn das neue Feld kleiner ist
		if (size < m_fieldsizes[field])
		{
			// Zeiger auf den Punkt nach dem aktuellen Feld setzen
			framedata += size;

			// Restliche Größe nach den alten Datensatz
			unsigned int remain_size = m_size - size - getPosition(field);

			// nachfolgende Daten um Differenz nach vorne schieben
			moveData( framedata, remain_size, (size - m_fieldsizes[field]) );

			// neue Gesamtgröße merken
			m_size = m_size - m_fieldsizes[field] + size;

			// neue Feldgröße merken
			m_fieldsizes[field] = size;

			// Framedaten um die Differenz verkleinern
			m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * m_size );
		}
		else
		// Wenn das neue Feld größer ist
		if (size > m_fieldsizes[field])
		{
			// neue Gesamtgröße merken
			m_size = m_size - (m_fieldsizes[field] - size);

			// Framedaten um die Differenz vergrößern
			m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * m_size );

			// Kopiere den Datenzeiger erneut
			framedata = m_data;

			// Setze den Zeiger auf die Position
			framedata += getPosition(field);

			// Zeiger auf den Punkt nach dem aktuellen Feld setzen
			framedata += m_fieldsizes[field];

			// Restliche Größe nach den alten Datensatz
			unsigned int remain_size = m_size - m_fieldsizes[field] - getPosition(field);

			// nachfolgende Daten um Differenz nach hinten schieben
			moveData( framedata, remain_size, (size - m_fieldsizes[field]) );

			// Zeiger auf das aktuelle Feld setzen
			framedata -= m_fieldsizes[field];

			// neue Feldgröße merken
			m_fieldsizes[field] = size;
		}
	}

	void NetworkFrame::moveData(unsigned char* data, unsigned int size, int offset)
	{
		if (offset > 0)
		{
			// Nix machen, wenn beide angaben gleich sind.
			if (size == (unsigned int)offset) return;

			// Berechne Datengröße
			unsigned int datasize = size - offset;

			// Zwischenspeicher der Größe |value|
			unsigned char* tmp = (unsigned char*)calloc(datasize, sizeof(unsigned char));

			// Kopiere die Daten in den Zwischenspeicher
			memcpy(tmp, data, datasize);

			// Bewege Datenzeiger nach hinten
			data += offset;

			// Kopiere die Daten zurück
			memcpy(data, tmp, datasize);

			// Lösche die Temporären Daten
			free(tmp);
		}
		else
		if (offset < 0)
		{
			// Nix machen, wenn beide angaben gleich sind.
			if (size == (unsigned int)(offset * -1)) return;

			// Berechne Datengröße
			unsigned int datasize = size + offset;

			// Zwischenspeicher der Größe |value|
			unsigned char* tmp = (unsigned char*)calloc(datasize, sizeof(unsigned char));

			// Setze den Zeiger auf den Anfang der Daten
			data -= offset;

			// Kopiere die Daten in den Zwischenspeicher
			memcpy(tmp, data, datasize);

			// Bewege Datenzeiger nach hinten
			data += offset;

			// Kopiere die Daten zurück
			memcpy(data, tmp, datasize);

			free(tmp);
		}
	}

	size_t NetworkFrame::write(FILE * stream)
	{
		return fwrite( getData(), sizeof(char), getSize(), stream );
	}

	void NetworkFrame::set(const unsigned int field, NetworkFrame &data)
	{
		// Ein anderes NetworkFrame-Objekt soll als ein Feld eingefügt werden.
		// 1. Verschiebe alle Felder nach hinten
		// 0 1 2 [0 1 2] 4 5 6 = 0 1 2 [0 1 2] 6 7 8
		// field = 3

		set(field, data.getData(), data.getSize());
		map<unsigned int, unsigned int> mcopy = m_fieldsizes;
		map<unsigned int, unsigned int> &fmap = data.getFieldSizeMap();

		// Neue Felder kopieren
		for (unsigned int i = 0; i < fmap.size(); i++)
		{
			m_fieldsizes[i + field] = fmap[i];
		}

		// Alte Felder verschoben kopieren
		for (unsigned int i = field + 1; i < mcopy.size(); i++)
		{
			m_fieldsizes[i + (fmap.size() - 1)] = mcopy[i];
		}
	}

	void NetworkFrame::debug() const
	{
		map<unsigned int, unsigned int> mcopy = m_fieldsizes;

		// Neue Felder kopieren
		for (unsigned int i = 0; i < mcopy.size(); i++)
		{
			unsigned char *data = get(i);
			cout << i << ": ";
			debugData(data, mcopy[i]);
			cout << endl;
		}
	}

	void NetworkFrame::debugData(unsigned char* data, unsigned int size) const
	{
		//dtn::Utils::getLogger() << "|";
		for (unsigned int i = 0; i < size; i++)
		{
			printf("%02X", (*data));
			//Utils::getLogger() << (*data);
			data++;
		}
		//dtn::Utils::getLogger() << "|" << endl;
	}
}
}
