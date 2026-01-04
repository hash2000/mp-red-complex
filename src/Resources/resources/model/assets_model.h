#pragma once
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

