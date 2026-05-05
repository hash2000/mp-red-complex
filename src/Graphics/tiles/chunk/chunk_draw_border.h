#pragma once
#include "Graphics/tiles/chunk/chunk_draw_data.h"
#include <QColor>

class ChunkDrawBorder : public ChunkDrawData {
public:
	ChunkDrawBorder();
	~ChunkDrawBorder();

	// Цвет рамки
	void setBorderColor(const QColor& color);
	QColor borderColor() const;

protected:
	void onRebuildVertices(std::vector<float>& vertices) override;
	void onRebuildVerticesAttributes(QOpenGLVertexArrayObject& vao, const std::vector<float>& vertices) override;
	GLenum drawMode() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
