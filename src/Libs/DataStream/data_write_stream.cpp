#include "Libs/DataStream/data_write_stream.h"
#include <zlib.h>

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

void DataWriteStream::writeBlock(const void* input) {
	const auto decompSize = decompressedSize();
	auto prevpos = position();
	position(dataOffset());

	if (compressed()) {
		// Буфер для сжатых данных
		std::vector<uint8_t> compressed;
		compressed.resize(compressBound(decompSize)); // Максимальный возможный размер
		auto zdata = compressed.data();

		z_stream zs;
		zs.total_in = decompSize;
		zs.avail_in = decompSize;
		zs.next_in = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(input));
		zs.total_out = 0;
		zs.avail_out = compressed.size();
		zs.next_out = zdata;
		zs.zalloc = Z_NULL;
		zs.zfree = Z_NULL;
		zs.opaque = Z_NULL;

		auto ret = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
		if (ret != Z_OK) {
			throw std::runtime_error(QString("deflateInit failed: %1")
				.arg(ret)
				.toStdString());
		}

		ret = deflate(&zs, Z_FINISH);
		if (ret != Z_STREAM_END) {
			deflateEnd(&zs);
			throw std::runtime_error("deflate failed");
		}

		const auto compSize = zs.total_out;
		deflateEnd(&zs);

		writeRaw(zdata, compSize);

		compressedSize(compSize);
		decompressedSize(decompSize);
	}
	else {
		writeRaw(input, decompSize);
	}

	position(prevpos);
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
