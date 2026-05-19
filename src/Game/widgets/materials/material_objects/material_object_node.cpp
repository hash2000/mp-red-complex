#include "Game/widgets/materials/material_objects/material_object_node.h"
#include <QUuid>

namespace MaterialObjectsRoles {
	constexpr int Type = Qt::UserRole;
	constexpr int Path = Qt::UserRole + 1;
	constexpr int Guid = Qt::UserRole + 2;
};


MaterialObjectNode::MaterialObjectNode(QStandardItem* item)
	: _item(item) {
}

MaterialObjectNode::~MaterialObjectNode() = default;

QUuid MaterialObjectNode::guid() const {
	if (!_item) {
		return QUuid();
	}

	return _item->data(MaterialObjectsRoles::Guid).toUuid();
}

QString MaterialObjectNode::path() const {
	if (!_item) {
		return QString();
	}

	return _item->data(MaterialObjectsRoles::Path).toString();
}

MaterialObjectTypes MaterialObjectNode::type() const {
	if (!_item) {
		return MaterialObjectTypes::Undefined;
	}

	return static_cast<MaterialObjectTypes>(_item->data(MaterialObjectsRoles::Type).toInt());
}

QStandardItem* MaterialObjectNode::appendNode(QStandardItem* parent, const QString& name, MaterialObjectTypes type) {
	if (!parent) {
		return nullptr;
	}

	const auto isRoot = parent->parent() == nullptr;
	const auto parentType = isRoot ? MaterialObjectTypes::Directory :
		static_cast<MaterialObjectTypes>(parent->data(MaterialObjectsRoles::Type).toInt());

	if (parentType != MaterialObjectTypes::Directory) {
		return nullptr;
	}

	auto subItem = new QStandardItem("new_item");
	parent->appendRow(subItem);

	subItem->setData(static_cast<int>(type), MaterialObjectsRoles::Type);
	subItem->setData(QUuid::createUuid(), MaterialObjectsRoles::Guid);
	subItem->setData(QString(), MaterialObjectsRoles::Path);
	subItem->setIcon(getIcon(type));

	return subItem;
}


QIcon MaterialObjectNode::getIcon(MaterialObjectTypes type) {
	switch (type) {
	case MaterialObjectTypes::Directory: return QIcon::fromTheme(QIcon::ThemeIcon::FolderOpen);
	case MaterialObjectTypes::VertexShader: return QIcon::fromTheme(QIcon::ThemeIcon::DocumentPrintPreview);
	case MaterialObjectTypes::FragmentShader: return QIcon::fromTheme(QIcon::ThemeIcon::CameraVideo);
	case MaterialObjectTypes::Texture: return QIcon::fromTheme(QIcon::ThemeIcon::DriveOptical);
	}

	return QIcon::fromTheme(QIcon::ThemeIcon::WindowClose);
}
