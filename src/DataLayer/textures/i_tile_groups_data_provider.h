#pragma once
#include <QString>
#include <QList>
#include <QUuid>
#include <optional>

// Структура группы тайлов
struct TileGroup {
	QUuid id;              // Уникальный ID группы
	QString name;          // Отображаемое имя группы
	QString texturePath;   // Путь к текстуре тайлового набора
	QList<int> tileIds;    // Список ID тайлов в группе
};

// Интерфейс провайдера данных для групп тайлов
class ITileGroupsDataProvider {
public:
	virtual ~ITileGroupsDataProvider() = default;

	// Загрузить все группы для указанного текстуры
	virtual QList<TileGroup> loadGroups(const QString& texturePath) const = 0;

	// Сохранить группу
	virtual bool saveGroup(const TileGroup& group) = 0;

	// Удалить группу по ID
	virtual bool deleteGroup(const QUuid& groupId) = 0;

	// Удалить все группы для текстуры
	virtual bool deleteGroupsForTexture(const QString& texturePath) = 0;
};
