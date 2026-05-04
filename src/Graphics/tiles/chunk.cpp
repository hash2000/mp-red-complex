#include "Graphics/tiles/chunk.h"
#include "Graphics/textures/texture_atlas.h"
#include "Graphics/tiles/chunk/chunk_draw_tileset.h"
#include "Graphics/tiles/chunk/chunk_draw_border.h"
#include "Graphics/gl_check_errors.h"
#include <qdebug.h>
#include <vector>

class Chunk::Private {
public:
	Private(Chunk* parent)
		: q(parent) {
	}

	Chunk* q;
	QSize chunkSize = { Chunk::kDefaultChunkSize, Chunk::kDefaultChunkSize };
	int layersCount = 0;
	float zLevel = 0.0f;
	int chunkX = 0;
	int chunkZ = 0;
	std::unique_ptr<ChunkDrawBorder> border;
	std::vector<std::unique_ptr<ChunkDrawTileset>> tiles;
	std::shared_ptr<TextureAtlas> tileset;
	bool initialized = false;
};

Chunk::Chunk(int layers)
: d(std::make_unique<Private>(this)) {
	d->layersCount = layers;
	d->tiles.resize(layers);

	for (int i = 0; i < layers; i++) {
		d->tiles[i] = std::make_unique<ChunkDrawTileset>();
	}

	d->border = std::make_unique<ChunkDrawBorder>();
}

Chunk::~Chunk() = default;

void Chunk::setChunkPosition(int chunkX, int chunkZ) {
	for (int i = 0; i < d->layersCount; i++) {
		d->tiles[i]->setPosition(chunkX, chunkZ);
	}

	d->border->setPosition(chunkX, chunkZ);
	d->chunkX = chunkX;
	d->chunkZ = chunkZ;
}

void Chunk::setBorderColor(const QColor& color) {
	d->border->setBorderColor(color);
}

QColor Chunk::borderColor() const {
	return d->border->borderColor();
}

void Chunk::setZLevel(float level) {
	d->zLevel = level;
}

float Chunk::zLevel() const {
	return d->zLevel;
}

int Chunk::chunkX() const {
	return d->chunkX;
}

int Chunk::chunkZ() const {
	return d->chunkZ;
}

void Chunk::setTileset(std::shared_ptr<TextureAtlas> tileset) {
	for (int i = 0; i < d->layersCount; i++) {
		d->tiles[i]->setTileset(tileset);
	}

	d->tileset = tileset;
}

void Chunk::setChunkSize(const QSize& size) {
	for (int i = 0; i < d->layersCount; i++) {
		d->tiles[i]->setChunkSize(size);
	}

	d->border->setChunkSize(size);
	d->chunkSize = size;
}

QSize Chunk::chunkSize() const {
	return d->chunkSize;
}

void Chunk::setTile(int worldX, int worldZ, int tileId, int layer) {
	if (layer < 0 || layer >= d->layersCount) {
		return;
	}

	d->tiles[layer]->setTile(worldX, worldZ, tileId);
}

int Chunk::getTile(int worldX, int worldZ, int layer) const {
	if (layer < 0 || layer >= d->layersCount) {
		return -1;
	}

	return d->tiles[layer]->getTile(worldX, worldZ);
}

void Chunk::rebuild() {
	for (int i = 0; i < d->layersCount; i++) {
		d->tiles[i]->rebuild();
	}

	d->border->rebuild();
}

void Chunk::render() {
	if (!d->tileset || !d->tileset->isLoaded()) {
		return;
	}

	d->tileset->bind();
	GL_CHECK_ERRORS();


	for (int i = 0; i < d->layersCount; i++) {
		d->tiles[i]->render();
	}

	d->tileset->unbind();
}

void Chunk::renderBorder() {
	d->border->render();
}

float Chunk::worldMinX() const {
	return static_cast<float>(d->chunkX * d->chunkSize.width());
}

float Chunk::worldMaxX() const {
	return static_cast<float>((d->chunkX + 1) * d->chunkSize.width());
}

float Chunk::worldMinZ() const {
	return static_cast<float>(d->chunkZ * d->chunkSize.height());
}

float Chunk::worldMaxZ() const {
	return static_cast<float>((d->chunkZ + 1) * d->chunkSize.height());
}
