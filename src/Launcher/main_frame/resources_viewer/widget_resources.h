#pragma once
#include "Base/format.h"
#include "Base/from.h"

enum class WidgetResource {
	Hex,
	Text,
	Int,
	Msg,
};

template <> class Format<WidgetResource> {
  static QString format(const WidgetResource &value) {
    switch (value) {
    case WidgetResource::Hex:
      return QStringLiteral("hex");
    case WidgetResource::Text:
      return QStringLiteral("txt");
    case WidgetResource::Int:
			return QStringLiteral("int");
    case WidgetResource::Msg:
			return QStringLiteral("msg");
    }
    return QStringLiteral("Unknown ressource");
  }
};

template <> struct From<WidgetResource> {
  static std::optional<WidgetResource> from(const QString &value) {
    if (value == QStringLiteral("hex")) {
      return WidgetResource::Hex;
    } else if (value == QStringLiteral("txt")) {
      return WidgetResource::Text;
    } else if (value == QStringLiteral("int")) {
      return WidgetResource::Int;
    } else if (value == QStringLiteral("msg")) {
      return WidgetResource::Msg;
    }
    return std::nullopt;
  }
};
