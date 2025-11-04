#pragma once
#include "Launcher/resources/game/endianness.h"
#include <QString>
#include <iostream>
#include <cstdint>


class DataStreamBase: public std::streambuf {
public:
	virtual ~DataStreamBase() = default;

	virtual size_t size() const = 0;

	virtual std::streambuf::int_type underflow();

	virtual void position(size_t pos) = 0;

	virtual size_t position() const;

	virtual void skip(size_t pos) = 0;

	virtual void read(uint8_t* destination, size_t size);

	EndiannessId endianness() const;

	void endianness(EndiannessId end);

	size_t remains() const;

	const QString name() const;

	void name(const QString &name);

private:
	EndiannessId _endianness = EndiannessId::Big;
	QString _name;
};
