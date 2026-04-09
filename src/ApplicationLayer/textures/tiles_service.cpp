#include "ApplicationLayer/textures/tiles_service.h"
#include "DataLayer/textures/i_tile_groups_data_provider.h"
#include <QUuid>
#include <QHash>
#include <optional>

class TilesService::Private {
public:
	Private(TilesService* parent) : q(parent) {}
	TilesService* q;
	ITileGroupsDataProvider* tileGroupsDataProvider = nullptr;

	// Кэш загруженных групп
	mutable QHash<QString, QList<TileGroup>> groupsCache;

	// Получить кэшированные группы или загрузить
	const QList<TileGroup>& getGroupsCached(const QString& texturePath) {
		auto it = groupsCache.find(texturePath);
		if (it != groupsCache.end()) {
			return it.value();
		}
		
		// Загружаем из провайдера
		groupsCache[texturePath] = tileGroupsDataProvider->loadGroups(texturePath);
		return groupsCache[texturePath];
	}

	// Инвалидировать кэш для текстуры
	void invalidateCache(const QString& texturePath) {
		groupsCache.remove(texturePath);
	}
};

TilesService::TilesService(
	ITileGroupsDataProvider* tileGroupsDataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->tileGroupsDataProvider = tileGroupsDataProvider;
}

TilesService::~TilesService() = default;

QList<TileGroup> TilesService::getGroups(const QString& texturePath) const {
	return d->getGroupsCached(texturePath);
}

std::optional<TileGroup> TilesService::getGroupContainingTile(const QString& texturePath, int tileId) const {
	const auto& groups = d->getGroupsCached(texturePath);
	for (const auto& group : groups) {
		if (group.tileIds.contains(tileId)) {
			return group;
		}
	}
	return std::nullopt;
}

QUuid TilesService::createGroup(const QString& texturePath, const QString& name, const QList<int>& tileIds) {
	if (tileIds.isEmpty()) {
		return QUuid();
	}

	TileGroup group;
	group.id = QUuid::createUuid();
	group.name = name;
	group.tileIds = tileIds;

	const bool success = d->tileGroupsDataProvider->saveGroup(texturePath, group);
	if (success) {
		d->invalidateCache(texturePath);
		emit groupsChanged(texturePath);
		return group.id;
	}

	return QUuid();
}

bool TilesService::updateGroup(const QString& texturePath, const TileGroup& group) {
	const bool success = d->tileGroupsDataProvider->saveGroup(texturePath, group);
	if (success) {
		d->invalidateCache(texturePath);
		emit groupsChanged(texturePath);
	}
	return success;
}

bool TilesService::deleteGroup(const QUuid& groupId) {
	// Находим группу по ID
	for (auto it = d->groupsCache.begin(); it != d->groupsCache.end(); ++it) {
		auto& groups = it.value();
		for (int i = 0; i < groups.size(); ++i) {
			if (groups[i].id == groupId) {
				const QString texturePath = it.key();
				// Удаляем из списка
				groups.removeAt(i);
				// TODO: сохранить через провайдер (нужен метод deleteGroup)
				d->invalidateCache(texturePath);
				emit groupsChanged(texturePath);
				return true;
			}
		}
	}

	return false;
}

bool TilesService::deleteGroupsForTexture(const QString& texturePath) {
	const bool success = d->tileGroupsDataProvider->deleteGroupsForTexture(texturePath);
	if (success) {
		d->invalidateCache(texturePath);
		emit groupsChanged(texturePath);
	}
	return success;
}
