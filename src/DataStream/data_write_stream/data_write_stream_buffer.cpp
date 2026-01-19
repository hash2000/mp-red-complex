#include "DataStream/data_write_stream/data_write_stream_buffer.h"
#include "data_write_stream_buffer.h"

DataWriteStreamBuffer::DataWriteStreamBuffer(QByteArray* target)
: _buffer(target ? target : &_ownedBuffer) {
	_pos = _buffer->size();
}

void DataWriteStreamBuffer::writeRaw(const void* src, size_t size) {
	const char* p = static_cast<const char*>(src);
	const int newSize = static_cast<int>(_pos + size);

	if (newSize > _buffer->size()) {
		_buffer->resize(newSize);
	}

	std::memcpy(_buffer->data() + _pos, p, size);
	_pos += size;
}

size_t DataWriteStreamBuffer::size() const {
	return _buffer->size();
}

void DataWriteStreamBuffer::position(size_t pos) {
	_pos = pos;
	if (_pos > _buffer->size()) {
		_buffer->resize(static_cast<int>(_pos));
	}
}

size_t DataWriteStreamBuffer::position() const {
	return _pos;
}

void DataWriteStreamBuffer::skip(size_t n) {
	position(_pos + n);
}

const QByteArray& DataWriteStreamBuffer::data() const {
	return *_buffer;
}

QByteArray DataWriteStreamBuffer::takeData() {
	return std::exchange(_ownedBuffer, {});
}

void DataWriteStreamBuffer::clear() {
	_buffer->clear();
	_pos = 0;
}
