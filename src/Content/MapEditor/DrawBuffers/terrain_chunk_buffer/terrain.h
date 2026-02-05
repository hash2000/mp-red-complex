#pragma once
#include "Base/container_view.h"
#include <QOpenGLFunctions>
#include <vector>

class TerrainChunk {
public:
	static const int CHUNK_SIZE = 32;

public:
	TerrainChunk(int x0, int z0, int width, int depth);

	float height(int x, int z) const;
	void height(int x, int z, float h);

	bool walkable(int x, int z) const;
	void walkable(int x, int z, bool w);

	int width() const;
	int depth() const;

	bool inbox(int x, int z) const;

	void x(int v);
	int x() const;

	void z(int v);
	int z() const;

private:
	int _x;
	int _z;
	int _width;
	int _depth;
	std::vector<float> _heights;
	std::vector<uint8_t> _walkable;
	bool _dirty = true;
};

struct TerrainChunkKey {
	int x;
	int z;
	bool operator==(const TerrainChunkKey& other) const {
		return x == other.x && z == other.z;
	}

	static TerrainChunkKey fromWorld(int worldX, int worldZ) {
		return TerrainChunkKey{
				worldX / TerrainChunk::CHUNK_SIZE,
				worldZ / TerrainChunk::CHUNK_SIZE
		};
	}
};

namespace std {
	template<> struct hash<TerrainChunkKey> {
		size_t operator()(const TerrainChunkKey& k) const {
			return static_cast<size_t>(k.x) * 31 - 1 + k.z;
		}
	};
}
