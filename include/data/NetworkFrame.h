#include "config.h"

#ifndef NETWORKFRAME_H_
#define NETWORKFRAME_H_

#include <map>
#include <sys/types.h>
#include <stdio.h>
#include <string>

using namespace std;

namespace dtn
{
namespace data
{
	class NetworkFrame
	{
		public:
			/**
			 * Create a new empty Frame with a length of zero.
			 */
			NetworkFrame();

			/**
			 * Create a new Frame with a copy of the given data.
			 * @param[in] data The data for the new frame.
			 * @param[in] size The length of the data.
			 */
			NetworkFrame(const unsigned char* data, const unsigned int size);

			/**
			 * Clean-up all the data of this object and free the data-array.
			 */
			~NetworkFrame();

			/**
			 * Copy constructor.
			 * @param[in] k The NetworkFrame to copy.
			 */
			NetworkFrame(const NetworkFrame& k);

			/**
			 * Copy constructor.
			 * @param[in] k The NetworkFrame to copy.
			 */
			NetworkFrame(const NetworkFrame* k);

			/**
			 * Operator for assign a NetworkFrame.
			 * @param[in] k The NetworkFrame to copy.
			 * @return The copy of the given NetworkFrame.
			 */
			NetworkFrame &operator=(const NetworkFrame &k);

			/**
			 * Append a new field with the given data at the end of the
			 * NetworkFrame.
			 * @param[in] data The data-array to be set.
			 * @param[in] size The length of the data-array.
			 * @return The field-index of the new field.
			 */
			unsigned int append(const unsigned char* data, const unsigned int size);

			/**
			 * Append a new field with a u_int64_t value encoded as a SDNV
			 * at the end of the NetworkFrame.
			 * @param[in] value The u_int64_t value to be set.
			 * @return The field-index of the new field.
			 */
			unsigned int append(const u_int64_t value);

			/**
			 * Append a new field with a int value encoded as a SDNV
			 * at the end of the NetworkFrame.
			 * @param[in] value The int value to be set.
			 * @return The field-index of the new field.
			 */
			unsigned int append(const int value);

			/**
			 * Append a new field with a unsigned int value encoded as a SDNV
			 * at the end of the NetworkFrame.
			 * @param[in] value The unsigned int value to be set.
			 * @return The field-index of the new field.
			 */
			unsigned int append(const unsigned int value);

			/**
			 * Append a new field with a char value at the end
			 * of the NetworkFrame.
			 * @param[in] value The char value to be set.
			 * @return The field-index of the new field.
			 */
			unsigned int append(const char value);

			/**
			 * Append a new field with a unsigned char value at the end
			 * of the NetworkFrame.
			 * @param[in] value The unsigned char value to be set.
			 * @return The field-index of the new field.
			 */
			unsigned int append(const unsigned char value);

			/**
			 * Append a new field with a string value at the end
			 * of the NetworkFrame.
			 * @param[in] value The string value to be set.
			 * @return The field-index of the new field.
			 */
			unsigned int append(const string value);

			/**
			 * Append a new field with a double value at the end of the
			 * NetworkFrame.
			 * @param[in] value The double value to be set.
			 * @returns The field-index of the new field.
			 */
			unsigned int append(const double value);

			/**
			 * Get method for the field-size-mapping of this NetworkFrame.
			 * @returns A reference of the field-size-mapping.
			 */
			map<unsigned int, unsigned int> &getFieldSizeMap();

			/**
			 * Set the field-size-mapping to a existing mapping.
			 * @param[in] mapping A mapping for the data.
			 */
			void setFieldSizeMap(map<unsigned int, unsigned int> mapping);

			/**
			 * Get method for the pointer of the data-array.
			 * @return A pointer of the data-array.
			 */
			unsigned char* getData() const;

			/**
			 * Get method for the length of the data-array.
			 * @return The length of the data-array.
			 */
			unsigned int getSize() const;

			/**
			 * Get the length of a specific field.
			 * @param[in] field The field-index of the field.
			 * @return The length of a specific field.
			 */
			unsigned int getSize(const unsigned int field) const;

			/**
			 * Get method to return a pointer to a specific field.
			 * @param[in] field The field-index of the field.
			 * @return A pointer to the data-array at the position
			 * of the field.
			 */
			unsigned char* get(const unsigned int field) const;

			/**
			 * Get method for a u_int64_t value which is encoded as SDNV.
			 * @param[in] field The field-index of the field.
			 * @return A value decoded as u_int64_t.
			 */
			u_int64_t getSDNV(const unsigned int field) const;

			/**
			 * Get method for a unsigned char value of a specific field.
			 * @param[in] field The field-index of the field.
			 * @return A value decoded as unsigned char.
			 */
			unsigned char getChar(const unsigned int field) const;

			/**
			 * Get method for a string value of a specific field.
			 * @param[in] field The field-index of the field.
			 * @return A value decoded as string.
			 */
			string getString(const unsigned int field) const;

			/**
			 * Get method for a double value of a specific field.
			 * @param[in] field The field-index of the field.
			 * @return A value decoded as double.
			 */
			double getDouble(const unsigned int field) const;

			/**
			 * Get method to find the position of a specific field in a data-array.
			 * @param[in] field The field-index of the field.
			 * @return The begin of a field in the data-array.
			 */
			unsigned int getPosition(const unsigned int field) const;

			/**
			 * Copy a existing data-array to a field of this NetworkFrame.
			 * @param[in] field The field-index of the field.
			 * @param[in] data The data-array to be set.
			 * @param[in] size The size of the data-array.
			 */
			void set(const unsigned int field, const unsigned char* data, const unsigned int size);

			/**
			 * Set a u_int64_t value into a specific field, encoded as SDNV.
			 * @param[in] field The field-index of the field.
			 * @param[in] value The u_int64_t value to be set.
			 */
			void set(const unsigned int field, const u_int64_t value);

			/**
			 * Set a int value into a specific field, encoded as SDNV.
			 * @param[in] field The field-index of the field.
			 * @param[in] value The int value to be set.
			 */
			void set(const unsigned int field, const int value);

			/**
			 * Set a char value into a specific field.
			 * @param[in] field The field-index of the field.
			 * @param[in] value The char value to be set.
			 */
			void set(const unsigned int field, const char value);

			/**
			 * Set a string into a specific field.
			 * @param[in] field The field-index of the field.
			 * @param[in] value The string value to be set.
			 */
			void set(const unsigned int field, const string value);

			/**
			 * Set a unsigned int value into a specific field, encoded as SDNV.
			 * @param[in] field The field-index of the field.
			 * @param[in] value The unsigned int value to be set.
			 */
			void set(const unsigned int field, const unsigned int value);

			/**
			 * Set a unsigned char value into a specific field.
			 * @param[in] field The field-index of the field.
			 * @param[in] value The unsigned char value to be set.
			 */
			void set(const unsigned int field, const unsigned char value);

			/**
			 * Set a double value into a specific field.
			 * @param[in] field The field-index of the field.
			 * @param[in] value The double value to be set.
			 */
			void set(const unsigned int field, double value);

			/**
			 * Copy a existing NetworkFrame into the data-array of this NetworkFrame.
			 * @param[in] field The field-index of the field.
			 * @param[in] data A reference to the NetworkFrame with data to copy.
			 */
			void set(const unsigned int field, NetworkFrame &data);

			/**
			 * Remove a specific field from the NetworkFrame.
			 * @param[in] field The field-index of the field.
			 */
			void remove(const unsigned int field);

			/**
			 * Insert a new field into the NetworkFrame with the given
			 * field-index. All existing field are renumbered.
			 * @param[in] field The number of the new field.
			 */
			void insert(const unsigned int field);

			/**
			 * Write a NetworkFrame to a FILE stream.
			 * @param[in] stream A opened filestream.
			 * @return The size of written bytes.
			 */
			size_t write(FILE * stream);

			/**
			 * Change the size of a field to a specific value.
			 * @param[in] field The field-index of the field.
			 * @param[in] size The new size of the field.
			 */
			void changeSize(unsigned int field, unsigned int size);

#ifdef DO_DEBUG_OUTPUT
			void debug() const;
#endif

			/**
			 * re-new the value of m_size
			 */
			void updateSize();

		protected:
			NetworkFrame(unsigned char* data, unsigned int size, map<unsigned int, unsigned int> fieldsizes);

		private:
			unsigned char* m_data;

			/**
			 * Mapping between field-index and length of the field.
			 */
			map<unsigned int, unsigned int> m_fieldsizes;

			/**
			 * Full length of the NetworkFrame.
			 */
			unsigned int m_size;

			/**
			 * Method to move data to front or back of a data-array.
			 * @param[in,out] data Data-array to be changed.
			 * @param[in] size The length of the data-array.
			 * @param[in] offset The begin of the data to move.
			 */
			void moveData(unsigned char* data, unsigned int size, int offset);

#ifdef DO_DEBUG_OUTPUT
			void debugData(unsigned char* data, unsigned int size) const;
#endif
	};
}
}

#endif /*NETWORKFRAME_H_*/
