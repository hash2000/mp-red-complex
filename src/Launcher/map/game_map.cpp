#include "Launcher/map/game_map.h"
#include <random>
#include <chrono>

Tile& GameMap::getOrCreateTile(int worldX, int worldY) {
	int chunkX = worldX / CHUNK_SIZE;
	int chunkY = worldY / CHUNK_SIZE;
	int localX = (worldX % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
	int localY = (worldY % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

	ChunkKey key{ chunkX, chunkY };
	if (_chunks.find(key) == _chunks.end()) {
		generateChunk(chunkX, chunkY);
	}
	return _chunks[key][localY * CHUNK_SIZE + localX];
}

void GameMap::generateChunk(int chunkX, int chunkY) {
	ChunkKey key{ chunkX, chunkY };
	_chunks[key].resize(CHUNK_SIZE * CHUNK_SIZE);
	fillChunk(_chunks[key], chunkX, chunkY);
}

void GameMap::fillChunk(std::vector<Tile>& tiles, int chunkX, int chunkY) {
	static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> biomeDist(1, 15); // skip Empty (0)
	std::uniform_real_distribution<float> destructionDist(0.0f, 30.0f); // mostly intact

	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE; ++i) {
		Tile& t = tiles[i];
		t._biome = static_cast<BiomeType>(biomeDist(rng));
		t._destruction = destructionDist(rng);

		// Пример: добавим ресурсы для некоторых биомов
		if (t._biome == BiomeType::IronOre || t._biome == BiomeType::GoldVein) {
			t._resources.push_back({ "Iron", 70.0f });
			t._resources.push_back({ "Nickel", 20.0f });
			t._resources.push_back({ "Copper", 10.0f });
		}
		else if (t._biome == BiomeType::Aquifer) {
			t._resources.push_back({ "Water", 95.0f });
			t._resources.push_back({ "Minerals", 5.0f });
		}
		// можно расширить
	}
}


const std::vector<Tile>& GameMap::getChunkTiles(int chunkX, int chunkY) {
	return _chunks.at(ChunkKey{ chunkX, chunkY });
}
