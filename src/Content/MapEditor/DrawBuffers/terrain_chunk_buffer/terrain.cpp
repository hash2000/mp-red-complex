#include "Content/MapEditor/DrawBuffers/terrain_chunk_buffer/terrain.h"

TerrainChunk::TerrainChunk(int x0, int z0, int width, int depth)
: _x(x0)
, _z(z0)
,	_width(width)
, _depth(depth) {
	_heights.resize(_width * _depth, 0.0f);
	// all walwable by default
	_walkable.resize(_width * _depth, 1); 
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

bool TerrainChunk::walkable(int x, int z) const
{
	if (inbox(x, z)) {
		return _walkable[z * _width + x] != 0;
	}

	return false;
}

void TerrainChunk::walkable(int x, int z, bool w)
{
	if (inbox(x, z)) {
		_walkable[z * _width + x] = w ? 1 : 0;
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
