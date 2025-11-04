#include "Launcher/resources/data_stream/data_stream_buffer.h"

DataStreamBuffer::DataStreamBuffer() {
}

size_t DataStreamBuffer::size() const {
  return _buffer.size();
}

void DataStreamBuffer::position(size_t pos) {
	auto buf = &_buffer.front();
	setg(buf, buf + pos, buf + _buffer.size());
}

void DataStreamBuffer::skip(size_t pos) {
	auto buf = &_buffer.front();
	setg(buf, gptr() + pos, buf + _buffer.size());
}
