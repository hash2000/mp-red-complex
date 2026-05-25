#include "Game/widgets/materials/material_objects/material_object_node.h"
#include <QUuid>

namespace MaterialObjectsRoles {
	constexpr int Type = Qt::UserRole;
	constexpr int Guid = Qt::UserRole + 1;
	constexpr int Path = Qt::UserRole + 2;
	constexpr int Color = Qt::UserRole + 3;
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

void MaterialObjectNode::setPath(const QString& value) {
	if (!_item) {
		return;
	}

	_item->setData(value, MaterialObjectsRoles::Path);
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

void MaterialObjectNode::setColor(const QColor& value) {
	if (!_item) {
		return;
	}

	_item->setData(value, MaterialObjectsRoles::Color);
}

QColor MaterialObjectNode::color() const {
	if (!_item) {
		return QColor::Invalid;
	}

	const auto value = _item->data(MaterialObjectsRoles::Color);
	if (!value.canConvert<QColor>()) {
		return QColor::Invalid;
	}

	return value.value<QColor>();
}


QStandardItem* MaterialObjectNode::appendNode(QStandardItem* parent, const QString& name, MaterialObjectTypes type) {
	if (!parent) {
		return nullptr;
	}

	auto subItem = new QStandardItem(name);
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
	case MaterialObjectTypes::Material: return QIcon::fromTheme(QIcon::ThemeIcon::ToolsCheckSpelling);
	case MaterialObjectTypes::MaterialRoot: return QIcon::fromTheme(QIcon::ThemeIcon::DialogInformation);
	case MaterialObjectTypes::BaseColor: return QIcon::fromTheme(QIcon::ThemeIcon::Battery);
	}

	return QIcon::fromTheme(QIcon::ThemeIcon::WindowClose);
}
