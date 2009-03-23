#include "emma/MeasurementJob.h"
#include "utils/Utils.h"

#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace emma
{
	MeasurementJob::MeasurementJob(unsigned char type, string cmd) : m_command(cmd), m_type(type), m_data(NULL), m_size(0)
	{
	}

	MeasurementJob::~MeasurementJob()
	{
		free(m_data);
	}

	void MeasurementJob::execute()
	{
		unsigned char buffer[1024];

		FILE *fp;
		int status;

		// execute the command
		fp = popen(m_command.c_str(), "r");

		// if handle is NULL then return.
		if (fp == NULL) return;

		int data = 0;
		int i = 0;

		// read data
		do {
			data = fgetc(fp);
			buffer[i] = (char)data;
			i++;
		} while (data != EOF);

		// close handle
		status = pclose(fp);

		if (status == -1) {
			/* Error reported by pclose() */

		} else {
			// copy new data
			m_size = i - 1;
			free(m_data);
			m_data = (unsigned char*)calloc(m_size, sizeof(char));
			memcpy(m_data, buffer, m_size);
		}
	}

	unsigned char MeasurementJob::getType()
	{
		return m_type;
	}

	unsigned char* MeasurementJob::getData()
	{
		return m_data;
	}

	unsigned int MeasurementJob::getLength()
	{
		return m_size;
	}
}
