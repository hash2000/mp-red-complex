#pragma once
#include "DataStream/data_write_stream.h"

class DataWriteStreamBuffer : public DataWriteStream {
public:
	explicit DataWriteStreamBuffer(QByteArray* target = nullptr);

	void writeRaw(const void* src, size_t size) override;

	size_t size() const override;
	void position(size_t pos) override;
	size_t position() const override;
	void skip(size_t n) override;

	const QByteArray& data() const;
	QByteArray takeData();

	void clear();

private:
	QByteArray* _buffer;
	QByteArray _ownedBuffer;
	size_t _pos = 0;
};
