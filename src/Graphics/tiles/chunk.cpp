#include "Graphics/tiles/chunk.h"
#include "Graphics/tiles/tileset.h"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
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
	int chunkSize = Chunk::kDefaultChunkSize;
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

	d->vao.create();
	d->vbo.create();
	d->borderVbo.create();

	// Инициализируем пустой буфер
	d->vao.bind();
	d->vbo.bind();
	d->vbo.allocate(nullptr, 0);

	// Настраиваем атрибуты
	// Атрибут 0: позиция (x, z) - 2 float
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));

	// Атрибут 1: UV координаты (u, v) - 2 float
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

	// Инициализируем данные для рамки (4 линии по периметру чанка)
	rebuildBorderVBO();

	d->vao.release();
	d->vbo.release();

	// Инициализируем данные тайлов
	d->tileData.resize(d->chunkSize * d->chunkSize, -1);
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

void Chunk::setChunkSize(int size) {
	if (size <= 0 || size > 64) {
		qWarning() << "Chunk: invalid chunk size" << size;
		return;
	}
	d->chunkSize = size;
	d->tileData.resize(d->chunkSize * d->chunkSize, -1);
	markDirty();
}

void Chunk::setTile(int localX, int localZ, int tileId) {
	if (localX < 0 || localX >= d->chunkSize || localZ < 0 || localZ >= d->chunkSize) {
		return;
	}
	d->tileData[localZ * d->chunkSize + localX] = tileId;
	markDirty();
}

int Chunk::getTile(int localX, int localZ) const {
	if (localX < 0 || localX >= d->chunkSize || localZ < 0 || localZ >= d->chunkSize) {
		return -1;
	}
	return d->tileData[localZ * d->chunkSize + localX];
}

void Chunk::markDirty() {
	d->dirty = true;
}

bool Chunk::isDirty() const {
	return d->dirty;
}

void Chunk::rebuild() {
	if (!d->initialized || !d->dirty) {
		return;
	}

	if (!d->tileset || !d->tileset->isInitialized()) {
		return;
	}

	std::vector<float> vertices;
	// Резервируем память: 6 вершин на тайл, 4 float на вершину
	vertices.reserve(d->chunkSize * d->chunkSize * 6 * 4);

	for (int z = 0; z < d->chunkSize; ++z) {
		for (int x = 0; x < d->chunkSize; ++x) {
			int tileId = d->tileData[z * d->chunkSize + x];
			if (tileId < 0) {
				continue; // Пустой тайл
			}

			auto tileData = d->tileset->getTileById(tileId);
			if (tileData.isEmpty()) {
				continue;
			}

			// Позиция тайла в мировых координатах
			float worldX = static_cast<float>(d->chunkX * d->chunkSize + x);
			float worldZ = static_cast<float>(d->chunkZ * d->chunkSize + z);

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

	d->vertexCount = static_cast<int>(vertices.size() / 4);

	// Обновляем VBO
	d->vao.bind();
	d->vbo.bind();
	d->vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));
	d->vbo.release();
	d->vao.release();

	d->dirty = false;
}

void Chunk::render() {
	if (!d->initialized || d->dirty || d->vertexCount == 0) {
		return;
	}

	d->vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, d->vertexCount);
	d->vao.release();
}

bool Chunk::isInitialized() const {
	return d->initialized;
}

float Chunk::worldMinX() const {
	return static_cast<float>(d->chunkX * d->chunkSize);
}

float Chunk::worldMaxX() const {
	return static_cast<float>((d->chunkX + 1) * d->chunkSize);
}

float Chunk::worldMinZ() const {
	return static_cast<float>(d->chunkZ * d->chunkSize);
}

float Chunk::worldMaxZ() const {
	return static_cast<float>((d->chunkZ + 1) * d->chunkSize);
}

void Chunk::rebuildBorderVBO() {
	if (!d->initialized) {
		return;
	}

	// Создаем 4 линии по периметру чанка (8 вершин, 2D позиция)
	// Линии: нижняя, правая, верхняя, левая
	std::vector<float> borderVertices;
	borderVertices.reserve(8 * 2); // 8 вершин, 2 float каждая

	float minX = static_cast<float>(d->chunkX * d->chunkSize);
	float maxX = static_cast<float>((d->chunkX + 1) * d->chunkSize);
	float minZ = static_cast<float>(d->chunkZ * d->chunkSize);
	float maxZ = static_cast<float>((d->chunkZ + 1) * d->chunkSize);

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

	d->borderVbo.bind();
	d->borderVbo.allocate(borderVertices.data(), static_cast<int>(borderVertices.size() * sizeof(float)));
	d->borderVbo.release();
}

void Chunk::renderBorder() {
	if (!d->initialized || d->borderVertexCount == 0) {
		return;
	}

	// Привязываем VAO, чтобы не нарушать состояние
	d->vao.bind();
	
	// Привязываем VBO рамки и настраиваем атрибут
	d->borderVbo.bind();
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
	
	glDrawArrays(GL_LINES, 0, d->borderVertexCount);
	
	glDisableVertexAttribArray(0);
	d->borderVbo.release();
	d->vao.release();
}
