#pragma once
#include "Base/format.h"
#include "Base/from.h"
#include <QObject>
#include <QString>

enum class AssetsViewItemRole : int {
	ContainerName = Qt::UserRole,
	FullPath = Qt::UserRole + 1,
	Suffix = Qt::UserRole + 2,
	Type = Qt::UserRole + 3,
	PathPart = Qt::UserRole + 4,
	DummyItem = Qt::UserRole + 4,
};

enum class AssetsViewItemType : unsigned char {
	Undefined,
	Container,
	Folder,
	File,
};

Q_DECLARE_METATYPE(AssetsViewItemType)

template <> struct Format<AssetsViewItemType> {
	static QString format(const AssetsViewItemType& value) {
		switch (value) {
		case AssetsViewItemType::Undefined:
			return QStringLiteral("Undefined");
		case AssetsViewItemType::Container:
			return QStringLiteral("Container");
		case AssetsViewItemType::Folder:
			return QStringLiteral("Folder");
		case AssetsViewItemType::File:
			return QStringLiteral("File");
		}
		return QStringLiteral("Undefined");
	}
};

template <> struct From<AssetsViewItemType> {
	static std::optional<AssetsViewItemType> from(const QString& value) {
		if (value == QStringLiteral("Undefined")) {
			return AssetsViewItemType::Undefined;
		} else if (value == QStringLiteral("Container")) {
			return AssetsViewItemType::Container;
		} else if (value == QStringLiteral("Folder")) {
			return AssetsViewItemType::Folder;
		} else if (value == QStringLiteral("File")) {
			return AssetsViewItemType::File;
		}
		return std::nullopt;
	}
};
