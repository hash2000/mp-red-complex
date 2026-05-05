#include "Graphics/tiles/chunk/chunk_draw_border.h"
#include "Graphics/gl_check_errors.h"

class ChunkDrawBorder::Private {
public:
	Private(ChunkDrawBorder* parent) : q(parent) {
	}

	ChunkDrawBorder* q;
	QColor borderColor = QColor(255, 0, 0, 255);
};

ChunkDrawBorder::ChunkDrawBorder()
	: d(std::make_unique<Private>(this)) {
}

ChunkDrawBorder::~ChunkDrawBorder() = default;


void ChunkDrawBorder::setBorderColor(const QColor& color) {
	d->borderColor = color;
}

QColor ChunkDrawBorder::borderColor() const {
	return d->borderColor;
}

void ChunkDrawBorder::onRebuildVertices(std::vector<float>& vertices) {
	const auto cs = chunkSize();
	const auto cx = chunkX();
	const auto cz = chunkZ();

	vertices.reserve(8 * 2); // 8 вершин, 2 float каждая

	float minX = static_cast<float>(cx * cs.width());
	float maxX = static_cast<float>((cx + 1) * cs.width());
	float minZ = static_cast<float>(cz * cs.height());
	float maxZ = static_cast<float>((cz + 1) * cs.height());

	qInfo() << "Rebuild border x(" << minX << "," << maxX << ") y(" << minZ << "," << maxZ << ")";

	// Нижняя линия: (minX, minZ) -> (maxX, minZ)
	vertices.push_back(minX); vertices.push_back(minZ);
	vertices.push_back(maxX); vertices.push_back(minZ);

	// Правая линия: (maxX, minZ) -> (maxX, maxZ)
	vertices.push_back(maxX); vertices.push_back(minZ);
	vertices.push_back(maxX); vertices.push_back(maxZ);

	// Верхняя линия: (maxX, maxZ) -> (minX, maxZ)
	vertices.push_back(maxX); vertices.push_back(maxZ);
	vertices.push_back(minX); vertices.push_back(maxZ);

	// Левая линия: (minX, maxZ) -> (minX, minZ)
	vertices.push_back(minX); vertices.push_back(maxZ);
	vertices.push_back(minX); vertices.push_back(minZ);
}

void ChunkDrawBorder::onRebuildVerticesAttributes(QOpenGLVertexArrayObject& vao, const std::vector<float>& vertices) {
	auto f = QOpenGLContext::currentContext()->functions();

	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
	GL_CHECK_ERRORS();
}

GLenum ChunkDrawBorder::drawMode() {
	return GL_LINES;
}
