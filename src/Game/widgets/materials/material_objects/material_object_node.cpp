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


QStandardItem* MaterialObjectNode::appendNode(QStandardItem* parent, MaterialObjectTypes type) {
	if (!parent) {
		return nullptr;
	}

	const auto name = getNameByType(type);
	const auto nameWithPrefix = QString("%1 %2")
		.arg(getPrefix(type))
		.arg(name);
	auto subItem = new QStandardItem(nameWithPrefix);
	parent->appendRow(subItem);

	subItem->setData(static_cast<int>(type), MaterialObjectsRoles::Type);
	subItem->setData(QUuid::createUuid(), MaterialObjectsRoles::Guid);
	subItem->setData(QString(), MaterialObjectsRoles::Path);

	return subItem;
}

QString MaterialObjectNode::getNameByType(MaterialObjectTypes type) {
	switch (type) {
	case MaterialObjectTypes::Directory: return "new_directory";
	case MaterialObjectTypes::VertexShader: return "new_vert_shader";
	case MaterialObjectTypes::FragmentShader: return "new_frag_shader";
	case MaterialObjectTypes::Texture: return "new_texture";
	case MaterialObjectTypes::Material: return "new_material";
	case MaterialObjectTypes::MaterialRoot: return "Материалы";
	case MaterialObjectTypes::BaseColor: return "new_albedo";
	}

	return "new_item";
}

QString MaterialObjectNode::getPrefix(MaterialObjectTypes type) {
	switch (type) {
	case MaterialObjectTypes::Directory: return "📁";
	case MaterialObjectTypes::VertexShader: return "🎨";
	case MaterialObjectTypes::FragmentShader: return "🔷";
	case MaterialObjectTypes::Texture: return "🖼️";
	case MaterialObjectTypes::Material: return "🧩";
	case MaterialObjectTypes::MaterialRoot: return "🔗";
	case MaterialObjectTypes::BaseColor: return "🌈";
	}

	return "➕";
}
