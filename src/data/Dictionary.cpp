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

		string Dictionary::get(unsigned int scheme, unsigned int ssp) const
		{
			return get(scheme) + ":" + get(ssp);
		}

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

		void Dictionary::read(unsigned char *data, unsigned int length)
		{
			char *dict_data = reinterpret_cast<char*>(data);

			// delete all.
			m_entrys.clear();

			string tmp;
			unsigned int position = 0;

			while (length > position)
			{
				tmp = dict_data;

				// save the dataset
				m_entrys[position] = tmp;

				dict_data += tmp.length() + 1;
				position += tmp.length() + 1;
			}
		}

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

		unsigned int Dictionary::write(unsigned char *data) const
		{
			unsigned int length = 0;
			string write_str;

			map<unsigned int, string, m_positionSort>::const_iterator iter = m_entrys.begin();

			while (iter != m_entrys.end())
			{
				write_str = (*iter).second;

				// write the string
				memcpy(data, write_str.data(), write_str.length());
				data += write_str.length();
				length += write_str.length();

				// write the delimiter
				data[0] = '\0';
				data++;
				length++;

				// get to the next item
				iter++;
			}


			return length;
		}

		pair<string, string> Dictionary::split(string uri) throw (exceptions::InvalidDataException)
		{
			pair<string, string> result;

			size_t splitter = uri.find(":", 0);

			// break out if there is not delimiter
			if (splitter == string::npos)
			{
				throw exceptions::InvalidDataException(uri + " is no valid URI");
			}

			result.first = uri.substr(0, splitter);
			result.second = uri.substr(splitter + 1, uri.length() - splitter);

			return result;
		}
	}
}
