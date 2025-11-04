#include "Launcher/resources/data_stream.h"
#include <algorithm>

std::streambuf::int_type DataStream::underflow() {

  if (gptr() == egptr()) {
      return traits_type::eof();
  }

  return traits_type::to_int_type(*gptr());
}

size_t DataStream::position() const {
  return gptr() - eback();
}

void DataStream::read(uint8_t *destination, size_t size) {
	sgetn((char*)destination, size);
	if (endianness() == EndiannessId::Big) {
		std::reverse(destination, destination + size);
	}
}

EndiannessId DataStream::endianness() const
{
    return _endianness;
}

void DataStream::endianness(EndiannessId end) {
	_endianness = end;
}

size_t DataStream::remains() const {
  return size() - position();
}

const QString DataStream::name() const {
  return _name;
}

void DataStream::name(const QString &name) {
	_name = name;
}

bool DataStream::compressed() const {
	return _isCompressed;
}

void DataStream::compressed(bool set) {
	_isCompressed = set;
}

uint32_t DataStream::decompressedSize() const {
	return _decompressedSize;
}

void DataStream::decompressedSize(uint32_t value) {
	_decompressedSize = value;
}

uint32_t DataStream::compressedSize() const {
	return _compressedSize;
}

void DataStream::compressedSize(uint32_t value) {
	_compressedSize = value;
}

uint32_t DataStream::dataOffset() const {
	return _dataOffset;
}

void DataStream::dataOffset(uint32_t value) {
	_dataOffset = value;
}
