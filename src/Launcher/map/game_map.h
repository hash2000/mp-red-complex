// GameMap.h
#pragma once
#include "externals/FastNoiseLite/FastNoiseLite.h"
#include "Launcher/map/tile.h"
#include <unordered_map>
#include <QPoint>

struct ChunkKey {
	int x, y;
	bool operator==(const ChunkKey& other) const { return x == other.x && y == other.y; }
};

namespace std {
	template<> struct hash<ChunkKey> {
		size_t operator()(const ChunkKey& k) const {
			return (static_cast<size_t>(k.x) << 16) ^ static_cast<size_t>(k.y);
		}
	};
}

class GameMap {
public:
	static constexpr int CHUNK_SIZE = 32;

	Tile& getOrCreateTile(int worldX, int worldY);
	void generateChunk(int chunkX, int chunkY);

	const std::vector<Tile>& getChunkTiles(int chunkX, int chunkY);

private:
	void fillChunk(std::vector<Tile>& chunkTiles, int chunkX, int chunkY);

private:
	std::unordered_map<ChunkKey, std::vector<Tile>> _chunks;
	FastNoiseLite _noise;
};
