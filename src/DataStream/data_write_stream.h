#pragma once
#include "DataStream/data.h"
#include <streambuf>
#include <bit>
#include <type_traits>
#include <cstdint>
#include <span>
#include <QString>
#include <QByteArray>
#include <memory>

class DataWriteStream : public std::streambuf {
public:
	virtual ~DataWriteStream() = default;

	virtual size_t size() const = 0;
	virtual void position(size_t pos) = 0;
	virtual size_t position() const = 0;
	virtual void skip(size_t n) = 0;
	virtual void writeRaw(const void* src, size_t size) = 0;

	template <typename T>
		requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
	void write(const T& value) {
		T tmp = value;
		applyEndianness(tmp);
		writeRaw(&tmp, sizeof(T));
	}

	template <detail::ReadOnlyByteBuffer Buffer>
	void write(const Buffer& buffer) {
		const size_t n = buffer.size();
		if (n == 0) return;
		writeRaw(buffer.data(), n);
	}

	void write(std::span<const std::byte> buffer);

	template <std::integral T>
	DataWriteStream& operator<<(const T& value)& {
		write(value);
		return *this;
	}

	void write(const QByteArray& ba);
	void write(const QString& str);

	Endianness endianness() const noexcept;
	void endianness(Endianness e) noexcept;

	const QString& name() const noexcept;
	void name(const QString& n);

	bool compressed() const noexcept;
	void compressed(bool c) noexcept;

	uint32_t decompressedSize() const noexcept;
	void decompressedSize(uint32_t v) noexcept;

	uint32_t compressedSize() const noexcept;
	void compressedSize(uint32_t v) noexcept;

	uint32_t dataOffset() const noexcept;
	void dataOffset(uint32_t v) noexcept;

protected:
	template <typename T>
	void applyEndianness(T& value) const noexcept {
		if constexpr (sizeof(T) <= 1) {
			return;
		}
		else {
			const bool is_little_native = std::endian::native == std::endian::little;
			const bool need_swap = _endianness == Endianness::Big;

			if (need_swap) {
				value = swapBytes(value);
			}
		}
	}

	template <typename T>
	static constexpr T swapBytes(T value) noexcept {
#if __cpp_lib_byteswap >= 202110L
		return std::byteswap(value);
#else
		return detail::byteswap_fallback(value);
#endif
	}

private:
	Endianness _endianness = Endianness::Big;
	QString _name;
	bool _isCompressed = false;
	uint32_t _decompressedSize = 0;
	uint32_t _compressedSize = 0;
	uint32_t _dataOffset = 0;
};
