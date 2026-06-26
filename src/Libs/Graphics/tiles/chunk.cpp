#include "Libs/Graphics/tiles/chunk.h"
#include "Libs/Graphics/textures/texture_atlas.h"
#include "Libs/Graphics/tiles/chunk/chunk_draw_tileset.h"
#include "Libs/Graphics/tiles/chunk/chunk_draw_border.h"
#include <vector>

class Chunk::Private {
public:
	Private(Chunk* parent)
		: q(parent) {
	}

	Chunk* q;
	QSize chunkSize = { Chunk::kDefaultChunkSize, Chunk::kDefaultChunkSize };
	float zLevel = 0.0f;
	int chunkX = 0;
	int chunkZ = 0;
	std::unique_ptr<ChunkDrawBorder> border;
	std::vector<std::unique_ptr<ChunkDrawData>> drawLayers;
	bool initialized = false;
};

Chunk::Chunk()
: d(std::make_unique<Private>(this)) {
	d->border = std::make_unique<ChunkDrawBorder>();
}

Chunk::~Chunk() = default;

void Chunk::setChunkPosition(int chunkX, int chunkZ) {
	for (const auto &layer : d->drawLayers) {
		layer->setPosition(chunkX, chunkZ);
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

void Chunk::setChunkSize(const QSize& size) {
	for (const auto& layer : d->drawLayers) {
		layer->setChunkSize(size);
	}

	d->border->setChunkSize(size);
	d->chunkSize = size;
}

QSize Chunk::chunkSize() const {
	return d->chunkSize;
}

void Chunk::rebuild() {
	for (const auto& layer : d->drawLayers) {
		layer->rebuild();
	}

	d->border->rebuild();
}

void Chunk::render() {
	for (const auto& layer : d->drawLayers) {
		layer->render();
	}
}

void Chunk::renderBorder() {
	d->border->render();
}

void Chunk::showOnlyOneLayer(int layer) {
	if (layer < 0 || layer >= d->drawLayers.size()) {
		return;
	}

	for (int i = 0, count = d->drawLayers.size(); i < count; i++) {
		const auto isVisible = i == layer;
		d->drawLayers[i]->setVisible(isVisible);
	}
}

void Chunk::showAllLayers() {
	for (const auto& layer : d->drawLayers) {
		layer->setVisible(true);
	}
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

int Chunk::layerCount() const {
	return d->drawLayers.size();
}

int Chunk::addLayer(std::unique_ptr<ChunkDrawData>&& layer) {
	d->drawLayers.push_back(std::move(layer));
	return d->drawLayers.size() - 1;
}

void Chunk::removeLayer(int layer) {
	if (layer < 0 || layer >= d->drawLayers.size()) {
		return;
	}

	d->drawLayers.erase(d->drawLayers.begin() + layer);
}
