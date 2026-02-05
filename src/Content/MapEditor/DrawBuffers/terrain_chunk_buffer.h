#pragma once
#include "Content/MapEditor/DrawBuffers/terrain_chunk_buffer/terrain.h"
#include "Content/MapEditor/Draw/draw_buffer.h"
#include "Base/container_view.h"
#include <unordered_map>
#include <memory>

class TerrainChunkDrawBuffer : public DrawBuffer {
public:
	TerrainChunkDrawBuffer(int widthChunks, int depthChunks);

	void render(DrawProgram* program, QOpenGLFunctions* fn) override;

	TerrainChunk* get(int x, int z) const;

	bool inbox(int x, int z) const;

	auto terrains() const {
		return make_deref_view(_terrain);
	}

private:
	TerrainChunk* getOrCreateChunk(int x, int z);
	void buildChunk(TerrainChunk& chunk);

private:
	std::unordered_map<TerrainChunkKey, std::unique_ptr<TerrainChunk>> _terrain;
	int _vertexes{ 0 };
	int _widthChunks;
	int _depthChunks;
};
