#include "DataStream/data_write_stream.h"


void DataWriteStream::write(std::span<const std::byte> buffer) {
	if (!buffer.empty()) {
		writeRaw(buffer.data(), buffer.size());
	}
}

void DataWriteStream::write(const QByteArray& ba) {
	writeRaw(ba.constData(), static_cast<size_t>(ba.size()));
}

void DataWriteStream::write(const QString& str) {
	const QByteArray utf8 = str.toUtf8();
	(*this) << static_cast<uint32_t>(utf8.size());
	writeRaw(utf8.constData(), utf8.size());
}

Endianness DataWriteStream::endianness() const noexcept {
	return _endianness;
}

void DataWriteStream::endianness(Endianness e) noexcept {
	_endianness = e;
}

const QString& DataWriteStream::name() const noexcept {
	return _name;
}

void DataWriteStream::name(const QString& n) {
	_name = n;
}

bool DataWriteStream::compressed() const noexcept {
	return _isCompressed;
}

void DataWriteStream::compressed(bool c) noexcept {
	_isCompressed = c;
}

uint32_t DataWriteStream::decompressedSize() const noexcept {
	return _decompressedSize;
}

void DataWriteStream::decompressedSize(uint32_t v) noexcept {
	_decompressedSize = v;
}

uint32_t DataWriteStream::compressedSize() const noexcept {
	return _compressedSize;
}

void DataWriteStream::compressedSize(uint32_t v) noexcept {
	_compressedSize = v;
}

uint32_t DataWriteStream::dataOffset() const noexcept {
	return _dataOffset;
}

void DataWriteStream::dataOffset(uint32_t v) noexcept {
	_dataOffset = v;
}
