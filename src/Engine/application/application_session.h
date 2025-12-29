#pragma once
#include "Base/format.h"
#include "Base/from.h"

enum AppSession {
  Test,
  ResourcesView,
};

template <> struct Format<AppSession> {
  static QString format(const AppSession &value) {
    switch (value) {
    case AppSession::Test:
      return QStringLiteral("Test");
    case AppSession::ResourcesView:
      return QStringLiteral("ResourcesView");
    default:
      return QString();
    }
  }
};

template <> struct From<AppSession> {
  static std::optional<AppSession> from(const QString &value) {
    if (value == QStringLiteral("Test")) {
      return AppSession::Test;
    }
    else if (value == QStringLiteral("ResourcesView")) {
      return AppSession::ResourcesView;
    }

    return std::nullopt;
  }
};
