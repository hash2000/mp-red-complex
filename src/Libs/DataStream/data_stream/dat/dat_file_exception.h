#pragma once
#include "Libs/Base/exception.h"

class DatFileException : public Exception {
public:
	DatFileException(const QString& filePath, const QString& msg)
	 : Exception(QString("DatFile: ") + filePath + " error: " + msg) {
	}
};
