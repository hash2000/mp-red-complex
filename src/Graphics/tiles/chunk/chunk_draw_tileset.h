#pragma once
#include "Graphics/tiles/chunk/chunk_draw_data.h"

class TextureAtlas;

class ChunkDrawTileset : public ChunkDrawData {
public:
	ChunkDrawTileset();
	~ChunkDrawTileset();

	void setTileset(std::shared_ptr<TextureAtlas> tileset);

	// Установить тайл по локальным координатам (0..chunkSize-1)
	void setTile(int worldX, int worldZ, int tileId);
	int getTile(int worldX, int worldZ) const;

protected:
	void onRebuildVertices(std::vector<float>& vertices) override;
	void onRebuildVerticesAttributes(QOpenGLVertexArrayObject& vao, const std::vector<float>& vertices) override;
	bool onAfterRender() override;
	void onBeforeRender() override;
	GLenum drawMode() override;
	void onChunkSizeChanged(const QSize& size) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
