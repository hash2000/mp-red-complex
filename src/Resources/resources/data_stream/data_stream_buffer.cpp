#include "Resources/resources/data_stream/data_stream_buffer.h"

DataStreamBuffer::DataStreamBuffer(std::vector<char> buffer)
	: _buffer(std::move(buffer)) {
	applyBuffer();
}

DataStreamBuffer::DataStreamBuffer(std::span<const char> view)
	: _buffer(view.begin(), view.end()) {
	applyBuffer();
}

void DataStreamBuffer::applyBuffer() {
	if (!_buffer.empty()) {
  	char* buf = _buffer.data();
  	setg(buf, buf, buf + _buffer.size());  // [beg, cur, end)
  } else {
  	setg(nullptr, nullptr, nullptr);
  }

	compressedSize(_buffer.size());
	decompressedSize(_buffer.size());
	compressed(false);
}

size_t DataStreamBuffer::size() const {
  return _buffer.size();
}

void DataStreamBuffer::position(size_t pos) {
	if (pos > _buffer.size()) {
    throw std::out_of_range("DataStreamBuffer::position: pos > size");
  }

	auto buf = &_buffer.front();
	setg(buf, buf + pos, buf + _buffer.size());
}

void DataStreamBuffer::skip(size_t pos) {
  if (static_cast<size_t>(egptr() - gptr()) < pos) {
    throw std::runtime_error("DataStreamBuffer::skip: not enough data");
  }

  gbump(static_cast<int>(pos));
}
