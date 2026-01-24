#include "Content/Shared/DrawBuffers/terrain_chunk_buffer/terrain.h"

TerrainChunk::TerrainChunk(int x0, int z0, int width, int depth)
: _x(x0)
, _z(z0)
,	_width(width)
, _depth(depth) {
	_heights.resize(_width * _depth, 0.0f);
	_terrain.resize(_width * _depth);
	generate();
}

float TerrainChunk::height(int x, int z) const
{
	if (inbox(x, z)) {
		return _heights[z * _width + x];
	}
	return 0.0f;
}

void TerrainChunk::height(int x, int z, float h)
{
	if (inbox(x, z)) {
		_heights[z * _width + x] = h;
	}
}

TerrainBlockInfo* TerrainChunk::get(int x, int z) {
	if (!inbox(x, z)) {
		return nullptr;
	}

	return _terrain[z * _width + x].get();
}

void TerrainChunk::generate() {
	for (size_t i = 0; i < _width * _depth; i++)
	{
		auto info = std::make_unique<TerrainBlockInfo>();

		_terrain[i] = std::move(info);
	}
}

int TerrainChunk::width() const {
	return _width;
}

int TerrainChunk::depth() const {
	return _depth;
}

bool TerrainChunk::inbox(int x, int z) const {
	return x >= 0 && x < _width && z >= 0 && z < _depth;
}

void TerrainChunk::x(int v) {
	_x = v;
}

int TerrainChunk::x() const {
	return _x;
}

void TerrainChunk::z(int v) {
	_z = v;
}

int TerrainChunk::z() const {
	return _z;
}
