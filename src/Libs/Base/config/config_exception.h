#pragma once
#include "Libs/Base/exception.h"

class ConfigException : public Exception {
public:
	ConfigException(const QString& msg)
	: Exception(QString("Config Excepton: ") + msg) {
	}
};
