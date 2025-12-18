#pragma once
#include "Base/format.h"
#include "Base/from.h"

enum class WidgetResource {
	Hex,
	Text,
	Int,
	Msg,
	Sve,
	Bio,
	Gcd,
	Gam,
	Pro,
	Frm,
	Pal,
};

template <> struct Format<WidgetResource> {
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
		case WidgetResource::Sve:
			return QStringLiteral("sve");
		case WidgetResource::Bio:
			return QStringLiteral("bio");
		case WidgetResource::Gcd:
			return QStringLiteral("gcd");
		case WidgetResource::Gam:
			return QStringLiteral("gam");
		case WidgetResource::Pro:
			return QStringLiteral("pro");
		case WidgetResource::Frm:
			return QStringLiteral("frm");
		case WidgetResource::Pal:
			return QStringLiteral("pal");
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
    } else if (value == QStringLiteral("sve")) {
      return WidgetResource::Sve;
    } else if (value == QStringLiteral("bio")) {
      return WidgetResource::Bio;
    } else if (value == QStringLiteral("gcd")) {
      return WidgetResource::Gcd;
    } else if (value == QStringLiteral("gam")) {
      return WidgetResource::Gam;
    } else if (value == QStringLiteral("pro")) {
      return WidgetResource::Pro;
    } else if (value == QStringLiteral("frm")) {
      return WidgetResource::Frm;
    } else if (value == QStringLiteral("fr0")) {
      return WidgetResource::Frm;
    } else if (value == QStringLiteral("fr1")) {
      return WidgetResource::Frm;
    } else if (value == QStringLiteral("fr2")) {
      return WidgetResource::Frm;
    } else if (value == QStringLiteral("fr3")) {
      return WidgetResource::Frm;
    } else if (value == QStringLiteral("fr4")) {
      return WidgetResource::Frm;
    } else if (value == QStringLiteral("fr5")) {
      return WidgetResource::Frm;
    } else if (value == QStringLiteral("pal")) {
      return WidgetResource::Pal;
    }
    return std::nullopt;
  }
};
