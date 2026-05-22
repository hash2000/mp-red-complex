#pragma once
#include <QStandardItem>
#include <QIcon>
#include <QColor>
#include <QUuid>

enum class MaterialObjectTypes {
	Undefined,
	VertexShader,
	FragmentShader,
	Directory,
	Texture,
	MaterialRoot,
	Material,
	BaseColor,
};

class MaterialObjectNode {
public:
	MaterialObjectNode(QStandardItem* item);
	~MaterialObjectNode();

	QUuid guid() const;
	MaterialObjectTypes type() const;

	void setPath(const QString& value);
	QString path() const;

	void setColor(const QColor& value);
	QColor color() const;

public:
	static QStandardItem* appendNode(QStandardItem* parent, const QString& name, MaterialObjectTypes type);
	static QIcon getIcon(MaterialObjectTypes type);

private:
	QStandardItem* _item = nullptr;
};
