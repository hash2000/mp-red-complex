#include "Graphics/tiles/chunk/chunk_draw_data.h"
#include "Graphics/gl_check_errors.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLBuffer>

class ChunkDrawData::Private : public QOpenGLFunctions_3_3_Core {
public:
	Private(ChunkDrawData* parent) : q(parent) {
	}

	ChunkDrawData* q;
	QOpenGLBuffer vbo;
	QOpenGLVertexArrayObject vao;
	QSize chunkSize;
	int vertexCount = 0; // Количество вершин для отрисовки
	bool initialized = false;
	bool dirty = false;
	bool visible = true;
	int chunkX = 0;
	int chunkZ = 0;

	void initialize() {
		if (initialized) {
			return;
		}

		initialized = true;
		dirty = true;

		initializeOpenGLFunctions();
	}

};

ChunkDrawData::ChunkDrawData()
: d(std::make_unique<Private>(this)) {
}

ChunkDrawData::~ChunkDrawData() {
	if (d->vbo.isCreated()) {
		d->vbo.destroy();
	}
}

void ChunkDrawData::markDirty() {
	d->dirty = true;
}

bool ChunkDrawData::isDirty() const {
	return d->dirty;
}

void ChunkDrawData::setChunkSize(const QSize& size) {
	if ((size.width() <= 0 || size.width() > 64) || (size.height() <= 0 || size.height() > 64)) {
		qWarning() << "Chunk: invalid chunk size width" << size.width() << "size height" << size.height();
		return;
	}
	d->chunkSize = size;
	onChunkSizeChanged(size);
	markDirty();
}

QSize ChunkDrawData::chunkSize() const {
	return d->chunkSize;
}

int ChunkDrawData::vertexCount() const {
	return d->vertexCount;
}

void ChunkDrawData::setPosition(int chunkX, int chunkZ) {
	d->chunkX = chunkX;
	d->chunkZ = chunkZ;
}

int ChunkDrawData::chunkX() const {
	return d->chunkX;
}

int ChunkDrawData::chunkZ() const {
	return d->chunkZ;
}

void ChunkDrawData::setVisible(bool value) {
	d->visible = value;
}

bool ChunkDrawData::visible() const {
	return d->visible;
}

void ChunkDrawData::render() {
	if (!d->visible || !d->initialized || d->dirty || d->vertexCount == 0) {
		return;
	}

	if (!onAfterRender()) {
		return;
	}

	d->vbo.bind();
	d->vao.bind();
	d->glDrawArrays(drawMode(), 0, d->vertexCount);
	GL_CHECK_ERRORS();

	d->vao.release();
	d->vbo.release();

	onBeforeRender();
}

void ChunkDrawData::rebuild() {
	if (!d->dirty) {
		return;
	}

	if (!d->initialized) {
		d->initialize();
	}

	if (!d->initialized) {
		return;
	}

	std::vector<float> vertices;
	onRebuildVertices(vertices);

	if (vertices.empty()) {
		return;
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

	onRebuildVerticesAttributes(d->vao, vertices);

	d->vao.release();
	d->vbo.release();

	d->vertexCount = vertices.size();
	d->dirty = false;
}
