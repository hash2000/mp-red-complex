#include "ApplicationLayer/textures/tiles_service.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "DataLayer/images/i_tile_groups_data_provider.h"
#include <QUuid>
#include <QHash>
#include <QFileInfo>
#include <optional>

class TilesService::Private {
public:
	Private(TilesService* parent) : q(parent) {}
	TilesService* q;
	ITileGroupsDataProvider* tileGroupsDataProvider = nullptr;
	TexturesService* texturesService = nullptr;

	// Кэш загруженных групп
	mutable QHash<QString, QList<TileGroup>> groupsCache;

	QList<int> selectedTileIds;
	std::optional<TileSetMetadata> metadata;

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
	TexturesService* texturesService,
	ITileGroupsDataProvider* tileGroupsDataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->tileGroupsDataProvider = tileGroupsDataProvider;
	d->texturesService = texturesService;
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
	if (tileIds.isEmpty() || !d->metadata) {
		return QUuid();
	}

	TileGroup group;
	group.id = QUuid::createUuid();
	group.name = name;
	group.tileIds = tileIds;

	const bool success = d->tileGroupsDataProvider->saveGroup(texturePath, group, d->metadata.value());
	if (success) {
		d->invalidateCache(texturePath);
		emit groupsChanged(texturePath);
		return group.id;
	}

	return QUuid();
}

bool TilesService::updateGroup(const QString& texturePath, const TileGroup& group) {
	if (!d->metadata) {
		return false;
	}

	const bool success = d->tileGroupsDataProvider->saveGroup(texturePath, group, d->metadata.value());
	if (success) {
		d->invalidateCache(texturePath);
		emit groupsChanged(texturePath);
	}
	return success;
}

bool TilesService::deleteGroup(const QUuid& groupId) {
	if (!d->metadata) {
		return false;
	}

	for (auto it = d->groupsCache.begin(); it != d->groupsCache.end(); it++) {
		auto& groups = it.value();
		for (int i = 0; i < groups.size(); i++) {
			if (groups[i].id == groupId) {
				const QString texturePath = it.key();
				groups.removeAt(i);
				d->tileGroupsDataProvider->deleteGroup(groupId, d->metadata.value());
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

std::optional<TileSetMetadata> TilesService::getTileSetMetadata(const QString& texturePath) const {
	const auto name = QFileInfo(texturePath).baseName();
	if (d->metadata.has_value() && d->metadata->fileName == name) {
		return d->metadata;
	}

	d->metadata = d->tileGroupsDataProvider->loadTileSetMetadata(texturePath);
	return d->metadata;
}

std::optional<TileSetMetadata> TilesService::getCurrentTileSetMetadata() const {
	return d->metadata;
}

void TilesService::setSelectionTiles(const QList<int>& tileIds) {
	d->selectedTileIds = tileIds;
	emit tilesSelectionChanged(d->selectedTileIds);
}

QList<int> TilesService::getSelectionTiles() const {
	return d->selectedTileIds;
}

std::optional<QPixmap> TilesService::getTilemap(const QString& tag) const {
	if (!d->metadata.has_value()) {
		return std::nullopt;
	}

	auto pixmap = d->texturesService->getTexture(d->metadata->fileName + ".png", ImageType::TileSets, tag);
	return pixmap;
}

QString TilesService::getTileSetName() const {
	if (!d->metadata.has_value()) {
		return QString();
	}

	return d->metadata->fileName;
}
