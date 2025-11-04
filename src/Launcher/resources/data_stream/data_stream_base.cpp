#include "Launcher/resources/data_stream/data_stream_base.h"
#include <algorithm>

std::streambuf::int_type DataStreamBase::underflow() {

  if (gptr() == egptr()) {
      return traits_type::eof();
  }

  return traits_type::to_int_type(*gptr());
}

size_t DataStreamBase::position() const {
  return gptr() - eback();
}

void DataStreamBase::read(uint8_t *destination, size_t size) {
	sgetn((char*)destination, size);
	if (endianness() == EndiannessId::Big) {
		std::reverse(destination, destination + size);
	}
}

EndiannessId DataStreamBase::endianness() const
{
    return _endianness;
}

void DataStreamBase::endianness(EndiannessId end) {
	_endianness = end;
}

size_t DataStreamBase::remains() const {
  return size() - position();
}

const QString DataStreamBase::name() const {
  return _name;
}

void DataStreamBase::name(const QString &name) {
	_name = name;
}
