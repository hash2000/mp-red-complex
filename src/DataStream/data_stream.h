#pragma once
#include "DataStream/data.h"
#include <QByteArray>
#include <QString>
#include <cstdint>
#include <qstringview.h>
#include <span>
#include <type_traits>
#include <streambuf>

class DataStream : public std::streambuf {
public:
  virtual ~DataStream() = default;

  virtual size_t size() const = 0;
  virtual void position(size_t pos) = 0;
  virtual size_t position() const;
  virtual void skip(size_t pos) = 0;
  virtual void readRaw(void *dst, size_t size);
	virtual void clear();

  template <typename T>
    requires(std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
  void read(T &value) noexcept {
    readRaw(&value, sizeof(T));
    applyEndianness(value);
  }

  template <detail::ByteBuffer Buffer> void read(Buffer &buffer) {
    const auto n = buffer.size();
    if (n == 0) {
      return;
    }
    readRaw(buffer.data(), n);
  }

  void read(std::span<std::byte> buffer) {
    if (!buffer.empty()) {
      readRaw(buffer.data(), buffer.size());
    }
  }

	void read(std::vector<std::byte> &buffer) {
    if (!buffer.empty()) {
      readRaw(buffer.data(), buffer.size());
    }
  }

  template <std::integral T> DataStream &operator>>(T &value) & {
    read(value);
    return *this;
  }

  template <std::integral T> [[nodiscard]] T get() {
    T v{};
    read(v);
    return v;
  }

  [[nodiscard]] uint8_t u8() { return get<uint8_t>(); }
  [[nodiscard]] int8_t i8() { return get<int8_t>(); }
  [[nodiscard]] uint16_t u16() { return get<uint16_t>(); }
  [[nodiscard]] int16_t i16() { return get<int16_t>(); }
  [[nodiscard]] uint32_t u32() { return get<uint32_t>(); }
  [[nodiscard]] int32_t i32() { return get<int32_t>(); }
  [[nodiscard]] uint64_t u64() { return get<uint64_t>(); }
  [[nodiscard]] int64_t i64() { return get<int64_t>(); }

  std::shared_ptr<DataStream> makeBlockAsStream();
  QByteArray readBlockAsQByteArray();

  Endianness endianness() const;
  void endianness(Endianness end);

  size_t remains() const;

  const QString name() const;
  void name(const QString &name);

  bool compressed() const;
  void compressed(bool set);

	QString containerName() const;
	void containerName(const QString &name);

	QString fullPath() const;

  uint32_t decompressedSize() const;
  void decompressedSize(uint32_t value);

  uint32_t compressedSize() const;
  void compressedSize(uint32_t value);

  uint32_t dataOffset() const;
  void dataOffset(uint32_t value);

	ContainerType type() const;
	void type(ContainerType val);

	void throwExceptionIsSizeIsTooLong(size_t size, const std::string& message);

protected:
  template <typename T> void applyEndianness(T &value) const noexcept {
    if constexpr (sizeof(T) <= 1) {
      return; // 1 байт — endianness не важен
    } else {
      const bool is_little_native = std::endian::native == std::endian::little;
      const bool need_swap =_endianness == Endianness::Big;

      if (need_swap) {
        value = swapBytes(value);
      }
    }
  }

  template <typename T> static constexpr T swapBytes(T value) noexcept {
#if __cpp_lib_byteswap >= 202110L
    return std::byteswap(value);
#else
    return detail::byteswap_fallback(value);
#endif
  }

private:
  void readBlock(void *output);

private:
  Endianness _endianness = Endianness::Big;
	ContainerType _type = ContainerType::Undefined;
  QString _name;
	QString _containerName;
  bool _isCompressed = false;
  uint32_t _decompressedSize = 0;
  uint32_t _compressedSize = 0;
  uint32_t _dataOffset = 0;
};
