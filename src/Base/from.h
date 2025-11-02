#pragma once
#include <QString>
#include <optional>

template <class T> struct From {
  static std::optional<T> from(const QString &value) = delete;
};
