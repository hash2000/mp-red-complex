#pragma once
#include <QStandardItem>
#include <QIcon>
#include <QUuid>

enum class MaterialObjectTypes {
	Undefined,
	VertexShader,
	FragmentShader,
	Directory,
	Texture
};

class MaterialObjectNode {
public:
	MaterialObjectNode(QStandardItem* item);
	~MaterialObjectNode();

	QUuid guid() const;
	QString path() const;
	MaterialObjectTypes type() const;

public:
	static QStandardItem* appendNode(QStandardItem* parent, const QString& name, MaterialObjectTypes type);
	static QIcon getIcon(MaterialObjectTypes type);

private:
	QStandardItem* _item = nullptr;
};
