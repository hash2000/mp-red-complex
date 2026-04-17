#include "Graphics/tiles/tileset.h"
#include "Graphics/textures/texture_atlas.h"

class Tileset::Private {
public:
	Private(Tileset* parent)
		: q(parent) {
	}

	Tileset* q;
	TextureAtlas* atlas = nullptr;
	int tilesCountX = 0;
	int tilesCountY = 0;
	int chuncSize = 0;
};

Tileset::Tileset()
	: d(std::make_unique<Private>(this)) {
}

Tileset::~Tileset() = default;

void Tileset::initialize(TextureAtlas* atlas, int tilesCountX, int tilesCountY) {
	d->atlas = atlas;
	d->tilesCountX = tilesCountX;
	d->tilesCountY = tilesCountY;
}

TileData Tileset::getTile(int tileX, int tileY) const {
	if (!d->atlas || !d->atlas->isLoaded()) {
		return TileData::empty();
	}

	if (tileX < 0 || tileX >= d->tilesCountX || tileY < 0 || tileY >= d->tilesCountY) {
		return TileData::empty();
	}

	TileData data;
	data.tileId = tileY * d->tilesCountX + tileX;
	data.region = d->atlas->getRegion(tileX, tileY);
	return data;
}

TileData Tileset::getTileById(int tileId) const {
	if (tileId < 0 || tileId >= totalTiles()) {
		return TileData::empty();
	}

	int tileX = tileId % d->tilesCountX;
	int tileY = tileId / d->tilesCountX;
	return getTile(tileX, tileY);
}

bool Tileset::isInitialized() const {
	return d->atlas != nullptr;
}

TextureAtlas* Tileset::atlas() const {
	return d->atlas;
}

int Tileset::tilesCountX() const {
	return d->tilesCountX;
}

int Tileset::tilesCountY() const {
	return d->tilesCountY;
}

int Tileset::totalTiles() const {
	return d->tilesCountX * d->tilesCountY;
}
