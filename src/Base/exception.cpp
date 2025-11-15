#include "Base/exception.h"

Exception::Exception(const QString &msg)
	: _error(msg.toStdString())
{
}

const char *Exception::what() const noexcept {
  return _error.what();
}
