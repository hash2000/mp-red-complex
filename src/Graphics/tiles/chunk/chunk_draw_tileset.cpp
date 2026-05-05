#include "Graphics/tiles/chunk/chunk_draw_tileset.h"
#include "Graphics/textures/texture_atlas.h"
#include "Graphics/gl_check_errors.h"

class ChunkDrawTileset::Private {
public:
	Private(ChunkDrawTileset* parent) : q(parent) {
	}

	ChunkDrawTileset* q;
	std::vector<int> tileData;
	std::shared_ptr<TextureAtlas> tileset;

	bool isCoordInChunk(int localX, int localZ) {
		const auto cs = q->chunkSize();
		const auto cx = q->chunkX() * cs.width();
		const auto cz = q->chunkZ() + cs.height();
		const auto result =
			localX >= cx && localX < (cx + cs.width()) ||
			localZ >= cx && localZ < (cx + cs.height());

		return result;
	}

	int toLocalChunkXCoord(int worldX) const {
		const int chunkX = worldX / q->chunkSize().width();
		const int localX = worldX % q->chunkSize().width();
		return localX;
	}

	int toLocalChunkZCoord(int worldZ) const {
		const int chunkX = worldZ / q->chunkSize().height();
		const int localX = worldZ % q->chunkSize().height();
		return localX;
	}

	int getTileDataIndex(int worldX, int worldZ) const {
		const auto localX = toLocalChunkXCoord(worldX);
		const auto localZ = toLocalChunkZCoord(worldZ);
		return localZ * q->chunkSize().width() + localX;
	}
};

ChunkDrawTileset::ChunkDrawTileset()
: d(std::make_unique<Private>(this)) {
}

ChunkDrawTileset::~ChunkDrawTileset() = default;

void ChunkDrawTileset::setTileset(std::shared_ptr<TextureAtlas> tileset) {
	d->tileset = tileset;
	markDirty();
}

void ChunkDrawTileset::setTile(int worldX, int worldZ, int tileId) {
	if (!d->isCoordInChunk(worldX, worldZ)) {
		return;
	}

	const auto index = d->getTileDataIndex(worldX, worldZ);
	d->tileData[index] = tileId;
	markDirty();
}

int ChunkDrawTileset::getTile(int worldX, int worldZ) const {
	if (!d->isCoordInChunk(worldX, worldZ)) {
		return -1;
	}

	const auto index = d->getTileDataIndex(worldX, worldZ);
	return d->tileData[index];
}

void ChunkDrawTileset::onChunkSizeChanged(const QSize& size) {
	d->tileData.resize(size.width() * size.height(), -1);
}

void ChunkDrawTileset::onRebuildVertices(std::vector<float>& vertices) {
	if (!d->tileset) {
		return;
	}

	const auto cs = chunkSize();
	const auto cx = chunkX();
	const auto cz = chunkZ();

	vertices.reserve(cs.width() * cs.height() * 6 * 4);
	for (int z = 0; z < cs.height(); z++) {
		for (int x = 0; x < cs.width(); x++) {
			int tileId = d->tileData[z * cs.width() + x];
			if (tileId < 0) {
				continue; // Пустой тайл
			}

			auto tileData = d->tileset->getTileById(tileId);
			if (tileData.isEmpty()) {
				continue;
			}

			// Позиция тайла в мировых координатах
			float worldX = static_cast<float>(cx * cs.width() + x);
			float worldZ = static_cast<float>(cz * cs.height() + z);

			// Два треугольника = 6 вершин
			// Треугольник 1: верхний левый, нижний левый, нижний правый
			// Верхний левый (x, z) -> UV: (u1, v2)
			vertices.push_back(worldX); vertices.push_back(worldZ);
			vertices.push_back(tileData.region.u1); vertices.push_back(tileData.region.v2);

			// Нижний левый (x, z+1) -> UV: (u1, v1)
			vertices.push_back(worldX); vertices.push_back(worldZ + 1.0f);
			vertices.push_back(tileData.region.u1); vertices.push_back(tileData.region.v1);

			// Нижний правый (x+1, z+1) -> UV: (u2, v1)
			vertices.push_back(worldX + 1.0f); vertices.push_back(worldZ + 1.0f);
			vertices.push_back(tileData.region.u2); vertices.push_back(tileData.region.v1);

			// Треугольник 2: верхний левый, нижний правый, верхний правый
			// Верхний левый (x, z) -> UV: (u1, v2)
			vertices.push_back(worldX); vertices.push_back(worldZ);
			vertices.push_back(tileData.region.u1); vertices.push_back(tileData.region.v2);

			// Нижний правый (x+1, z+1) -> UV: (u2, v1)
			vertices.push_back(worldX + 1.0f); vertices.push_back(worldZ + 1.0f);
			vertices.push_back(tileData.region.u2); vertices.push_back(tileData.region.v1);

			// Верхний правый (x+1, z) -> UV: (u2, v2)
			vertices.push_back(worldX + 1.0f); vertices.push_back(worldZ);
			vertices.push_back(tileData.region.u2); vertices.push_back(tileData.region.v2);
		}
	}
}

void ChunkDrawTileset::onRebuildVerticesAttributes(QOpenGLVertexArrayObject& vao, const std::vector<float>& vertices) {
	auto f = QOpenGLContext::currentContext()->functions();

	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(
		0,
		2,
		GL_FLOAT,
		GL_FALSE,
		4 * sizeof(float),
		nullptr
	);
	GL_CHECK_ERRORS();

	f->glEnableVertexAttribArray(1);
	f->glVertexAttribPointer(
		1,
		2,
		GL_FLOAT,
		GL_FALSE,
		4 * sizeof(float),
		(void*)(2 * sizeof(float))
	);
	GL_CHECK_ERRORS();
}

GLenum ChunkDrawTileset::drawMode() {
	return GL_TRIANGLES;
}

