#include "DataStream/data_stream.h"
#include "DataStream/data_stream/data_stream_buffer.h"
#include <zlib.h>
#include <sstream>

size_t DataStream::position() const {
  return gptr() - eback();
}

void DataStream::position(size_t pos) {

}

void DataStream::clear() {

}

void DataStream::readRaw(void *dst, size_t size) {
  if (size == 0) {
		return;
	}

  const auto n = sgetn(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(size));
  if (static_cast<size_t>(n) != size) {
    throw std::runtime_error(QString("DataStream::readRaw: unexpected EOF. Expected %1, read %2.")
			.arg(size)
			.arg(n)
			.toStdString());
  }
}

void DataStream::readBlock(void *output) {
	const auto decompSize = decompressedSize();
	const auto compSize = compressedSize();
	auto prevpos = position();
	position(dataOffset());

	if (compressed()) {
		std::vector<char> compressed;
		compressed.resize(compSize);
		auto zdata = reinterpret_cast<uint8_t*>(compressed.data());
		readRaw(zdata, compSize);

		z_stream zs;
		zs.total_in = compressed.size();
		zs.avail_in = compressed.size();
		zs.next_in = zdata;
		zs.total_out = decompSize;
		zs.avail_out = decompSize;
		zs.next_out = reinterpret_cast<uint8_t*>(output);
		zs.zalloc = Z_NULL;
		zs.zfree = Z_NULL;
		zs.opaque = Z_NULL;

		auto ret = inflateInit(&zs);
		if (ret != Z_OK) {
			throw std::runtime_error("inflateInit failed: " + std::to_string(ret));
		}

		ret = inflate(&zs, Z_FINISH);
		const auto ok = (ret == Z_STREAM_END) &&
      (zs.total_out == decompSize);
		inflateEnd(&zs);
		if (!ok) {
      throw std::runtime_error("inflate failed or size mismatch");
		}

	} else {
		readRaw(output, decompSize);
	}

	position(prevpos);
}

std::shared_ptr<DataStream> DataStream::makeBlockAsStream() {
	std::vector<char> buffer;
	buffer.resize(decompressedSize());
	auto data = reinterpret_cast<uint8_t*>(buffer.data());
	readBlock(data);
	auto block = std::make_shared<DataStreamBuffer>(std::move(buffer));
	block->name(name());
	block->decompressedSize(decompressedSize());
	block->compressedSize(decompressedSize());
	block->compressed(false);
	return block;
}

QByteArray DataStream::readBlockAsQByteArray() {
	QByteArray buffer;
	buffer.resize(decompressedSize());
	auto data = reinterpret_cast<uint8_t*>(buffer.data());
	readBlock(data);
	return buffer;
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

void DataStream::throwExceptionIsSizeIsTooLong(size_t size, const std::string& message) {
	if (size < remains()) {
		return;
	}

	std::stringstream s;
	s << message << ", ";
	s << name().toStdString();
	s << " pos: " << position();

	throw std::runtime_error(s.str());
}
