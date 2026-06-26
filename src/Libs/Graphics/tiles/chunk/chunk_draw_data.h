#pragma once
#include <QOpenGLVertexArrayObject>
#include <QSize>
#include <memory>
#include <vector>

class ChunkDrawData {
public:
	ChunkDrawData();
	~ChunkDrawData();

	void rebuild();

	void render();

	int vertexCount() const;

	void setChunkSize(const QSize& size);
	QSize chunkSize() const;

	void setPosition(int chunkX, int chunkZ);
	int chunkX() const;
	int chunkZ() const;

	/// Пометить как грязный (требует перестройки VBO)
	void markDirty();
	bool isDirty() const;

	void setVisible(bool value);
	bool visible() const;


protected:
	virtual void onRebuildVertices(std::vector<float>& vertices) = 0;
	virtual void onRebuildVerticesAttributes(QOpenGLVertexArrayObject& vao, const std::vector<float>& vertices) { }
	virtual bool onAfterRender() { return true; }
	virtual void onBeforeRender() { }
	virtual GLenum drawMode() = 0;
	virtual void onChunkSizeChanged(const QSize& size) {}

private:
	class Private;
	std::unique_ptr<Private> d;
};
