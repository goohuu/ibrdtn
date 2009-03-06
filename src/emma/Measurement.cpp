#include "emma/Measurement.h"

#include "data/BundleFactory.h"
#include "data/Bundle.h"
#include "data/SDNV.h"
#include <cstdlib>
#include <cstring>

using namespace dtn::data;

namespace emma
{
	Measurement::Measurement(unsigned int datasize, unsigned int jobs) : m_datasize(datasize), m_data(NULL), m_length(0)
	{
		unsigned int time = BundleFactory::getDTNTime();

		// Etwas Speicher reservieren
		m_data = (unsigned char*)calloc(datasize, sizeof(unsigned char));

		// Eventuell Größe anpassen
		need( SDNV::encoding_len( time ) + SDNV::encoding_len( jobs ) );

		// Zeiger kopieren
		char* data = (char*)m_data;

		// Zeitpunkt hinzufügen
		unsigned int len = SDNV::encode( time, data, SDNV::encoding_len( time ) );
		m_length += len;
		data += len;

		// Anzahl der Jobs hinzufügen
		m_length += SDNV::encode( jobs, data, SDNV::encoding_len( jobs ) );
	}

	Measurement::~Measurement()
	{
		free(m_data);
	}

	void Measurement::need(unsigned int needed)
	{
		if ( ( m_datasize - m_length ) < needed )
		{
			m_datasize += needed - ( m_datasize - m_length );
			m_data = (unsigned char*)realloc(m_data, m_datasize);
		}
	}

	void Measurement::add(GPSProvider &gps)
	{
		NetworkFrame f;
		f.append(gps.getTime());
		f.append(gps.getLatitude());
		f.append(gps.getLongitude());

		add( 1, (char*)f.getData(), f.getSize() );
	}

	void Measurement::add(MeasurementJob &job)
	{
		add( job.getType(), (char*)job.getData(), job.getLength() );
	}

	void Measurement::add(unsigned char type, double value)
	{
		add( type, (char*)&value, sizeof(double) );
	}

	void Measurement::add(unsigned char type, char* job_data, unsigned int job_length)
	{
		// Datensatzgröße
		unsigned int size = sizeof(char) + SDNV::encoding_len( job_length ) + job_length;

		// Eventuell Größe anpassen
		need( size );

		// Zeiger kopieren
		char* data = (char*)m_data;

		// An das Ende setzen
		data += m_length;

		// Messdatentyp
		data[0] = type;
		data++;

		// Datenlänge
		data += SDNV::encode( job_length, data, SDNV::encoding_len( job_length ) );

		// Messdaten
		memcpy( data, job_data, job_length );

		// Datengröße erweitern
		m_length += size;
	}

	unsigned char* Measurement::getData()
	{
		return m_data;
	}

	unsigned int Measurement::getLength()
	{
		return m_length;
	}
}
