#include "Graphics/tiles/chunk.h"
#include "Graphics/tiles/tileset.h"
#include "Graphics/gl_check_errors.h"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <qdebug.h>
#include <vector>



class Chunk::Private {
public:
	Private(Chunk* parent)
		: q(parent)
		, vbo(QOpenGLBuffer::VertexBuffer)
		, borderVbo(QOpenGLBuffer::VertexBuffer) {
	}

	Chunk* q;
	bool initialized = false;
	bool dirty = false;
	int chunkX = 0;
	int chunkZ = 0;
	QSize chunkSize = { Chunk::kDefaultChunkSize, Chunk::kDefaultChunkSize };
	Tileset* tileset = nullptr;

	// Данные тайлов чанка (хранятся как tileId)
	std::vector<int> tileData;

	// OpenGL объекты
	QOpenGLVertexArrayObject vao;
	QOpenGLBuffer vbo;
	int vertexCount = 0; // Количество вершин для отрисовки

	// VBO для рамки чанка (4 линии = 8 вершин, 2D позиция)
	QOpenGLBuffer borderVbo;
	int borderVertexCount = 0;

	bool isCoordInChunk(int localX, int localZ) {
		const auto cx = chunkX * chunkSize.width();
		const auto cz = chunkZ + chunkSize.height();
		const auto result =
			localX >= cx && localX < (cx + chunkSize.width()) ||
			localZ >= cx && localZ < (cx + chunkSize.height());

		return result;
	}

	int toLocalChunkXCoord(int worldX) const {
		const int chunkX = worldX / chunkSize.width();
		const int localX = worldX % chunkSize.width();
		return localX;
	}

	int toLocalChunkZCoord(int worldZ) const {
		const int chunkX = worldZ / chunkSize.height();
		const int localX = worldZ % chunkSize.height();
		return localX;
	}

	int getTileDataIndex(int worldX, int worldZ) const {
		const auto localX = toLocalChunkXCoord(worldX);
		const auto localZ = toLocalChunkZCoord(worldZ);
		return localZ * chunkSize.width() + localX;
	}
};

Chunk::Chunk()
	: d(std::make_unique<Private>(this)) {
}

Chunk::~Chunk() {
	if (d->initialized) {
		d->vbo.destroy();
		d->vao.destroy();
	}
}

void Chunk::initialize() {
	if (d->initialized) {
		return;
	}

	d->initialized = true;
	d->dirty = true;

	initializeOpenGLFunctions();
}

void Chunk::setChunkPosition(int chunkX, int chunkZ) {
	d->chunkX = chunkX;
	d->chunkZ = chunkZ;
}

int Chunk::chunkX() const {
	return d->chunkX;
}

int Chunk::chunkZ() const {
	return d->chunkZ;
}

void Chunk::setTileset(Tileset* tileset) {
	d->tileset = tileset;
	markDirty();
}

void Chunk::setChunkSize(const QSize& size) {
	if ((size.width() <= 0 || size.width() > 64) || (size.height() <= 0 || size.height() > 64)) {
		qWarning() << "Chunk: invalid chunk size width" << size.width() << "size height" << size.height();
		return;
	}
	d->chunkSize = size;
	d->tileData.resize(d->chunkSize.width() * d->chunkSize.height(), -1);
	markDirty();
}

void Chunk::setTile(int worldX, int worldZ, int tileId) {
	if (!d->isCoordInChunk(worldX, worldZ)) {
		return;
	}

	const auto index = d->getTileDataIndex(worldX, worldZ);
	d->tileData[index] = tileId;
	markDirty();
}

int Chunk::getTile(int worldX, int worldZ) const {
	if (!d->isCoordInChunk(worldX, worldZ)) {
		return -1;
	}

	const auto index = d->getTileDataIndex(worldX, worldZ);
	return d->tileData[index];
}

void Chunk::markDirty() {
	d->dirty = true;
}

bool Chunk::isDirty() const {
	return d->dirty;
}

void Chunk::rebuild() {
	if (!d->dirty) {
		return;
	}

	if (!d->initialized) {
		initialize();
	}

	if (!d->initialized) {
		return;
	}

	rebuildBorderVBO();

	if (d->tileset && d->tileset->isInitialized()) {
		rebuildVertexes();
	}

	d->dirty = false;
}

void Chunk::render() {
	if (!d->initialized || d->dirty || d->vertexCount == 0) {
		return;
	}

	auto f = QOpenGLContext::currentContext()->functions();

	d->vao.bind();
	f->glDrawArrays(GL_TRIANGLES, 0, d->vertexCount);
	GL_CHECK_ERRORS();
	d->vao.release();
}

void Chunk::renderBorder() {
	if (!d->initialized || d->borderVertexCount == 0) {
		return;
	}

	auto f = QOpenGLContext::currentContext()->functions();

	// Рисуем рамку без VAO — просто настраиваем атрибут позиции
	d->borderVbo.bind();

	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));

	f->glDrawArrays(GL_LINES, 0, d->borderVertexCount);

	f->glDisableVertexAttribArray(0);
	d->borderVbo.release();
}

bool Chunk::isInitialized() const {
	return d->initialized;
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

void Chunk::rebuildVertexes() {
	std::vector<float> vertices;
	// Резервируем память: 6 вершин на тайл, 4 float на вершину
	vertices.reserve(d->chunkSize.width() * d->chunkSize.height() * 6 * 4);
	int tileCount = 0;
	for (int z = 0; z < d->chunkSize.height(); z++) {
		for (int x = 0; x < d->chunkSize.width(); x++) {
			int tileId = d->tileData[z * d->chunkSize.width() + x];
			if (tileId < 0) {
				continue; // Пустой тайл
			}

			auto tileData = d->tileset->getTileById(tileId);
			if (tileData.isEmpty()) {
				continue;
			}

			tileCount++;

			// Позиция тайла в мировых координатах
			float worldX = static_cast<float>(d->chunkX * d->chunkSize.width() + x);
			float worldZ = static_cast<float>(d->chunkZ * d->chunkSize.height() + z);

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

	auto f = QOpenGLContext::currentContext()->functions();

	d->vertexCount = static_cast<int>(vertices.size() / 4);

	if (!d->vbo.isCreated()) {
		d->vbo.create();
	}

	d->vbo.bind();
	d->vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
	GL_CHECK_ERRORS();

	if (!d->vao.isCreated()) {
		d->vao.create();
	}

	d->vao.bind();

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

	d->vbo.release();
	d->vao.release();


	qInfo() << "Chunk::rebuild - chunk("
		<< d->chunkX << "x" << d->chunkSize.width() << ","
		<< d->chunkZ << "x" << d->chunkSize.height() << ")"
		<< "tiles:" << tileCount << "vertices:" << d->vertexCount;
}

void Chunk::rebuildBorderVBO() {
	if (!d->initialized) {
		return;
	}

	// Создаем 4 линии по периметру чанка (8 вершин, 2D позиция)
	// Линии: нижняя, правая, верхняя, левая
	std::vector<float> borderVertices;
	borderVertices.reserve(8 * 2); // 8 вершин, 2 float каждая

	float minX = static_cast<float>(d->chunkX * d->chunkSize.width());
	float maxX = static_cast<float>((d->chunkX + 1) * d->chunkSize.width());
	float minZ = static_cast<float>(d->chunkZ * d->chunkSize.height());
	float maxZ = static_cast<float>((d->chunkZ + 1) * d->chunkSize.height());

	qInfo() << "Rebuild border x(" << minX << "," << maxX << ") y(" << minZ << "," << maxZ << ")";

	// Нижняя линия: (minX, minZ) -> (maxX, minZ)
	borderVertices.push_back(minX); borderVertices.push_back(minZ);
	borderVertices.push_back(maxX); borderVertices.push_back(minZ);

	// Правая линия: (maxX, minZ) -> (maxX, maxZ)
	borderVertices.push_back(maxX); borderVertices.push_back(minZ);
	borderVertices.push_back(maxX); borderVertices.push_back(maxZ);

	// Верхняя линия: (maxX, maxZ) -> (minX, maxZ)
	borderVertices.push_back(maxX); borderVertices.push_back(maxZ);
	borderVertices.push_back(minX); borderVertices.push_back(maxZ);

	// Левая линия: (minX, maxZ) -> (minX, minZ)
	borderVertices.push_back(minX); borderVertices.push_back(maxZ);
	borderVertices.push_back(minX); borderVertices.push_back(minZ);

	d->borderVertexCount = 8;

	if (!d->borderVbo.isCreated()) {
		d->borderVbo.create();
	}

	d->borderVbo.bind();
	d->borderVbo.allocate(borderVertices.data(), static_cast<int>(borderVertices.size() * sizeof(float)));
	d->borderVbo.release();
}

