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
			m_data = (unsigned char*)calloc(0, sizeof(unsigned char));
		}

		NetworkFrame::NetworkFrame(const unsigned char* data, const unsigned int size)
		 : m_data(NULL), m_size(size)
		{
			m_data = (unsigned char*)calloc(size, sizeof(unsigned char));
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
			m_data = (unsigned char*)calloc(m_size, sizeof(unsigned char));
			memcpy(m_data, k.m_data, m_size);
			m_fieldsizes = k.m_fieldsizes;
		}

		NetworkFrame::NetworkFrame(const NetworkFrame* k)
		 : m_data(NULL), m_size(k->m_size)
		{
			m_data = (unsigned char*)calloc(m_size, sizeof(unsigned char));
			memcpy(m_data, k->m_data, m_size);
			m_fieldsizes = k->m_fieldsizes;
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
			unsigned int fieldcount = m_fieldsizes.size();

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
			// count of fields = new ID
			unsigned int fieldcount = m_fieldsizes.size();

			// coding length of value
			unsigned int size = data::SDNV::encoding_len(value);

			// make the Frame bigger for the new value
			m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * (m_size + size) );

			// copy the pointer
			unsigned char *framedata = m_data;

			// set pointer to the end of the old frame
			framedata += m_size;

			// copy data to the new frame
			data::SDNV::encode(value, framedata, size);

			// save new size
			m_size += size;

			// save field size
			m_fieldsizes[fieldcount] = size;

			// return the new field id
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
			// count of fields = new ID
			unsigned int fieldcount = m_fieldsizes.size();

			// make the Frame bigger for the new value
			m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * (m_size + size) );

			// copy the pointer
			unsigned char *framedata = m_data;

			// set pointer to the end of the old frame
			framedata += m_size;

			// copy data to the new frame
			memcpy(framedata, data, size);

			// save new size
			m_size += size;

			// save field size
			m_fieldsizes[fieldcount] = size;

			// return the new field id
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
			// count of fields = new ID
			unsigned int fieldcount = m_fieldsizes.size();

			unsigned int size = sizeof(unsigned char);

			// make the Frame bigger for the new value
			m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * (m_size + size) );

			// copy the pointer
			unsigned char *framedata = m_data;

			// set pointer to the end of the old frame
			framedata += m_size;

			// cyop data to the new frame
			framedata[0] = value;

			// save new size
			m_size += size;

			// save field size
			m_fieldsizes[fieldcount] = size;

			// return the new field id
			return fieldcount;
		}

		unsigned char* NetworkFrame::get(const unsigned int field) const
		{
			// copy the pointer
			unsigned char* data = m_data;

			// set pointer to the position of the field
			data += getPosition(field);

			// return the pointer to the field
			return data;
		}

		u_int64_t NetworkFrame::getSDNV(const unsigned int field) const
		{
			// return value
			u_int64_t ret_val = 0;

			// pointer to the data array of the field
			const unsigned char *data = get(field);

			// decode the SDNV
			data::SDNV::decode(data, data::SDNV::len(data), &ret_val);

			// return the value
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
			const unsigned char *data = get(field);
			return data[0];
		}

		string NetworkFrame::getString(const unsigned int field) const
		{
			// get pointer to the data array
			unsigned char *data = get(field);
			unsigned int length = getSize(field);

			// create a buffer for the string
			char *str_data = (char*)calloc(length + 1, sizeof(char));

			// copy string data
			memcpy(str_data, data, length);

			// append string end
			str_data[length] = '\0';

			// convert to a real string
			string ret = str_data;

			// free the buffer
			free(str_data);

			// return the string
			return ret;
		}

		void NetworkFrame::set(const unsigned int field, const u_int64_t value)
		{
			// get coding size
			unsigned int size = data::SDNV::encoding_len(value);

			// resize the field size
			changeSize( field, size );

			// copy the pointer
			unsigned char *framedata = m_data;

			// set the pointer to the field
			framedata += getPosition(field);

			// copy data to the field
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
			// resize the data field
			changeSize( field, sizeof(unsigned char) );

			// copy the pointer
			unsigned char *framedata = m_data;

			// set the pointer to the data field
			framedata += getPosition(field);

			// write the data
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
			// resize the data field
			changeSize( field, size );

			// copy the pointer
			unsigned char *framedata = m_data;

			// set the pointer to the data field
			framedata += getPosition(field);

			// write the data
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
			// copy the pointer of the data array
			unsigned char *framedata = m_data;

			// set the pointer to the right position
			framedata += getPosition(field);

			// if the new field should be smaller than the old one...
			if (size < m_fieldsizes[field])
			{
				// set the pointer to the right position
				framedata += size;

				// get the remaining size after the field
				unsigned int remain_size = m_size - size - getPosition(field);

				// move remaining data
				moveData( framedata, remain_size, (size - m_fieldsizes[field]) );

				// save new frame size
				m_size = m_size - m_fieldsizes[field] + size;

				// save new field size
				m_fieldsizes[field] = size;

				// shrink the frame
				m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * m_size );
			}
			else
			// if the new field should be bigger...
			if (size > m_fieldsizes[field])
			{
				// save new frame size
				m_size = m_size - (m_fieldsizes[field] - size);

				// grow the frame
				m_data = (unsigned char*)realloc( m_data, sizeof(unsigned char) * m_size );

				// copy the data pointer
				framedata = m_data;

				// set the pointer to the field
				framedata += getPosition(field);

				// set the pointer to the position after the field
				framedata += m_fieldsizes[field];

				// get the remaining size after the field
				unsigned int remain_size = m_size - m_fieldsizes[field] - getPosition(field);

				// move data to the end of the new frame
				moveData( framedata, remain_size, (size - m_fieldsizes[field]) );

				// set the pointer to the current field
				framedata -= m_fieldsizes[field];

				// save new field size
				m_fieldsizes[field] = size;
			}
		}

		void NetworkFrame::moveData(unsigned char* data, unsigned int size, int offset)
		{
			if (offset > 0)
			{
				// do nothing if both are equal.
				if (size == (unsigned int)offset) return;

				// calc data size
				unsigned int datasize = size - offset;

				// temporarily array of size <value>
				unsigned char* tmp = (unsigned char*)calloc(datasize, sizeof(unsigned char));

				// copy the data to the temporarily memory
				memcpy(tmp, data, datasize);

				// move the pointer to the end
				data += offset;

				// copy the data
				memcpy(data, tmp, datasize);

				// free the temporarily data array
				free(tmp);
			}
			else
			if (offset < 0)
			{
				// do nothing if both are equal.
				if (size == (unsigned int)(offset * -1)) return;

				// calc data size
				unsigned int datasize = size + offset;

				// temporarily array of size <value>
				unsigned char* tmp = (unsigned char*)calloc(datasize, sizeof(unsigned char));

				// move the pointer to the begin
				data -= offset;

				// copy the data to the temporarily memory
				memcpy(tmp, data, datasize);

				// move the pointer to the end
				data += offset;

				// copy the data
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
			// another NetworkFrame should be copied into this NetworkFrame.
			// 1. move all field to the end
			// 0 1 2 [0 1 2] 4 5 6 = 0 1 2 [0 1 2] 6 7 8
			// field = 3

			set(field, data.getData(), data.getSize());
			map<unsigned int, unsigned int> mcopy = m_fieldsizes;
			map<unsigned int, unsigned int> &fmap = data.getFieldSizeMap();

			// copy new fields
			for (unsigned int i = 0; i < fmap.size(); i++)
			{
				m_fieldsizes[i + field] = fmap[i];
			}

			// copy old moved fields
			for (unsigned int i = field + 1; i < mcopy.size(); i++)
			{
				m_fieldsizes[i + (fmap.size() - 1)] = mcopy[i];
			}
		}

	#ifdef DO_DEBUG_OUTPUT
		void NetworkFrame::debug() const
		{
			map<unsigned int, unsigned int> mcopy = m_fieldsizes;

			// print a header
			cout << "|";

			for (unsigned int i = 0; i < mcopy.size(); i++)
			{
				for (unsigned int j = 0; j < mcopy[i]; j++)
				{
					if (j == 0)
					{
						cout << i;
						if (i < 10) cout << " ";
					}
					else
						cout << "  ";
				}

				cout << "|";
			}

			cout << endl;

			for (unsigned int i = 0; i < mcopy.size(); i++)
			{
				unsigned char *data = get(i);
				cout << "|";
				debugData(data, mcopy[i]);
			}

			cout << "|" << endl;
		}

		void NetworkFrame::debugData(unsigned char* data, unsigned int size) const
		{
			for (unsigned int i = 0; i < size; i++)
			{
				printf("%02X", (*data));
				data++;
			}
		}
	#endif
	}
}
