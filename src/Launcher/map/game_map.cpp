#include "Launcher/map/game_map.h"
#include "Launcher/map/resources_generator.h"
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
	// Настройка шума (делается один раз при создании GameMap, но можно и здесь)
	static bool noiseInitialized = false;
	if (!noiseInitialized) {
		_noise.SetSeed(12345);
		_noise.SetFrequency(0.02f); // чем меньше — крупнее регионы
		_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
		_noise.SetFractalOctaves(3);
		_noise.SetFractalLacunarity(2.0f);
		_noise.SetFractalGain(0.5f);
		noiseInitialized = true;
	}

	std::uniform_real_distribution<float> destructionDist(0.0f, 30.0f);
	std::mt19937 rng(42 + chunkX * 1000 + chunkY); // детерминированный на чанк

	// Определим список биомов, которые участвуют в генерации
	std::vector<BiomeType> biomes = {
		BiomeType::Empty,
		BiomeType::Sand,
		BiomeType::Soil,
		BiomeType::Clay,
		BiomeType::Stone,
		BiomeType::Granite,
		BiomeType::Basalt,
		BiomeType::Limestone,
		BiomeType::Coal,
		BiomeType::IronOre,
		BiomeType::GoldVein,
		BiomeType::OilShale,
		BiomeType::Aquifer,
		BiomeType::GasPocket
		// исключаем DiamondPipe, TitaniumDeposit — они редкие
	};

	const int numBiomes = static_cast<int>(biomes.size());

	ResourcesGenerator resGen;

	for (int y = 0; y < CHUNK_SIZE; ++y) {
		for (int x = 0; x < CHUNK_SIZE; ++x) {
			int worldX = chunkX * CHUNK_SIZE + x;
			int worldY = chunkY * CHUNK_SIZE + y;

			// Получаем значение шума в диапазоне [0, 1]
			float noiseValue = _noise.GetNoise((float)worldX, (float)worldY);
			noiseValue = (noiseValue + 1.0f) * 0.5f; // [-1,1] → [0,1]

			// Выбираем биом по шуму
			float biomeIndexFloat = noiseValue * (numBiomes - 1e-5f); // избегаем выхода за пределы
			int biomeIndex = static_cast<int>(std::floor(biomeIndexFloat));

			Tile& t = tiles[y * CHUNK_SIZE + x];
			t._biome = biomes[biomeIndex];
			t._destruction = destructionDist(rng);
			t._resources = resGen.generate(t._biome, rng);
		}
	}

	// редкие биомы
	std::uniform_real_distribution<float> rareDist(0.0f, 1.0f);
	if (rareDist(rng) < 0.01f) {
		int rx = std::uniform_int_distribution<int>(0, CHUNK_SIZE - 1)(rng);
		int ry = std::uniform_int_distribution<int>(0, CHUNK_SIZE - 1)(rng);
		tiles[ry * CHUNK_SIZE + rx]._biome = BiomeType::DiamondPipe;
		tiles[ry * CHUNK_SIZE + rx]._resources.push_back({"Diamond", 100.0f});
	}
}

const std::vector<Tile>& GameMap::getChunkTiles(int chunkX, int chunkY) {
	return _chunks.at(ChunkKey{ chunkX, chunkY });
}
