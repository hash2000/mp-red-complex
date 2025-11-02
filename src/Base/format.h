#pragma once
#include <QString>

template<class T>
struct Format {
	static QString format(const T& value) = delete;
};
