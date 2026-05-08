#include "Graphics/tiles/chunk.h"
#include "Graphics/textures/texture_atlas.h"
#include "Graphics/tiles/chunk/chunk_draw_tileset.h"
#include "Graphics/tiles/chunk/chunk_draw_border.h"
#include "Graphics/tiles/render_queue.h"
#include "Graphics/tiles/material.h"
#include <vector>

struct ChunkLayer {
	std::unique_ptr<ChunkDrawData> drawData;
	Material* material = nullptr;
	bool visible = true;
};

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
	Material* borderMaterial = nullptr;
	std::vector<ChunkLayer> layers;
	bool initialized = false;
};

Chunk::Chunk()
: d(std::make_unique<Private>(this)) {
	d->border = std::make_unique<ChunkDrawBorder>();
}

Chunk::~Chunk() = default;

void Chunk::setChunkPosition(int chunkX, int chunkZ) {
	for (auto& layer : d->layers) {
		layer.drawData->setPosition(chunkX, chunkZ);
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
	for (auto& layer : d->layers) {
		layer.drawData->setChunkSize(size);
	}

	d->border->setChunkSize(size);
	d->chunkSize = size;
}

QSize Chunk::chunkSize() const {
	return d->chunkSize;
}

void Chunk::rebuild() {
	for (auto& layer : d->layers) {
		layer.drawData->rebuild();
	}

	d->border->rebuild();
}

void Chunk::render(RenderQueue& queue) {
	for (auto& layer : d->layers) {
		if (!layer.visible) {
			continue;
		}
		queue.addCommand(layer.material, layer.drawData.get(), d->zLevel);
	}
}

void Chunk::renderBorder(RenderQueue& queue) {
	queue.addCommand(d->borderMaterial, d->border.get(), d->zLevel);
}

void Chunk::showOnlyOneLayer(int layer) {
	if (layer < 0 || layer >= static_cast<int>(d->layers.size())) {
		return;
	}

	for (int i = 0, count = d->layers.size(); i < count; i++) {
		const auto isVisible = i == layer;
		d->layers[i].visible = isVisible;
	}
}

void Chunk::showAllLayers() {
	for (auto& layer : d->layers) {
		layer.visible = true;
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
	return d->layers.size();
}

int Chunk::addLayer(std::unique_ptr<ChunkDrawData>&& layer, Material* material) {
	d->layers.push_back({std::move(layer), material, true});
	return d->layers.size() - 1;
}

void Chunk::removeLayer(int layer) {
	if (layer < 0 || layer >= static_cast<int>(d->layers.size())) {
		return;
	}

	d->layers.erase(d->layers.begin() + layer);
}

Material* Chunk::layerMaterial(int layer) const {
	if (layer < 0 || layer >= static_cast<int>(d->layers.size())) {
		return nullptr;
	}
	return d->layers[layer].material;
}
