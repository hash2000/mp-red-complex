#pragma once
#include "Launcher/map/tile.h"
#include <QString>
#include <random>
#include <algorithm>
#include <vector>

class ResourcesGenerator {
public:
	ResourcesGenerator(float rareResourceChance = 0.15f);

  std::vector<ResourceElement> generate(BiomeType biome, std::mt19937& rng);

private:
  std::pair<std::vector<QString>, std::vector<QString>> getBaseAndRare(BiomeType biome);

private:
  const float _rareResourceChance;
};
