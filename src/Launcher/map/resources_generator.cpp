#include "Launcher/map/resources_generator.h"

ResourcesGenerator::ResourcesGenerator(float rareResourceChance)
: _rareResourceChance(rareResourceChance) {
}

std::pair<std::vector<QString>, std::vector<QString>> ResourcesGenerator::getBaseAndRare(BiomeType biome) {
	switch (biome) {
	case BiomeType::IronOre:
		return { {"Iron", "Nickel", "Copper"}, {"Platinum", "Cobalt"} };
	case BiomeType::GoldVein:
		return { {"Gold", "Silver", "Quartz"}, {"Diamond Dust", "Iridium"} };
	case BiomeType::Aquifer:
		return { {"Water", "Dissolved Minerals"}, {"Lithium Brine", "Helium-3"} };
	case BiomeType::OilShale:
		return { {"Oil", "Kerogen", "Gas"}, {"Uranium Traces", "Hydrogen Sulfide"} };
	case BiomeType::Coal:
		return { {"Coal", "Sulfur"}, {"Germanium", "Rare Earths"} };
	case BiomeType::Granite:
		return { {"Feldspar", "Quartz", "Mica"}, {"Tantalum", "Niobium"} };
	case BiomeType::Basalt:
		return { {"Pyroxene", "Olivine"}, {"Titanium", "Chromium"} };
	case BiomeType::Clay:
		return { {"Kaolinite", "Montmorillonite"}, {"Zircon", "Bauxite"} };
	case BiomeType::DiamondPipe:
		return { {"Diamond"}, {"Blue Diamond", "Carbonado"} };
	case BiomeType::TitaniumDeposit:
		return { {"Ilmenite", "Rutile"}, {"Vanadium", "Scandium"} };
	default:
		return { {}, {} };
	}
}

std::vector<ResourceElement> ResourcesGenerator::generate(BiomeType biome, std::mt19937& rng) {

	std::vector<ResourceElement> result;

	auto [baseResources, rareResources] = getBaseAndRare(biome);
	if (baseResources.empty()) {
		return result;
	}

	// 1. Определим, сколько базовых ресурсов использовать: от 1 до min(3, N)
	int numBase = std::min(3, static_cast<int>(baseResources.size()));
	// Можно всегда брать все, или случайно — выберем все для простоты
	std::vector<QString> selectedBase(baseResources.begin(), baseResources.begin() + numBase);

	// 2. Сгенерируем случайные веса и нормализуем их в 100%
	std::vector<float> weights(numBase);
	std::uniform_real_distribution<float> weightDist(0.1f, 1.0f); // избегаем нуля
	float totalWeight = 0.0f;
	for (int i = 0; i < numBase; ++i) {
		weights[i] = weightDist(rng);
		totalWeight += weights[i];
	}

	// 3. Преобразуем в проценты
	for (int i = 0; i < numBase; ++i) {
		float pct = (weights[i] / totalWeight) * 100.0f;
		result.push_back({ selectedBase[i], pct });
	}

	// 4. С шансом 15% добавим один редкий ресурс
	if (!rareResources.empty()) {
		std::uniform_real_distribution<float> rareChance(0.0f, 1.0f);
		if (rareChance(rng) < _rareResourceChance) {
			// 15% шанс
			// Выберем случайный редкий ресурс
			std::uniform_int_distribution<size_t> rareIdx(0, rareResources.size() - 1);
			QString rareName = rareResources[rareIdx(rng)];
			// Редкий ресурс "вытесняет" часть базовых — уменьшим общий % базовых до 90–99%
			std::uniform_real_distribution<float> rarePct(1.0f, 10.0f); // 1–10%
			float rarePercent = rarePct(rng);

			// Уменьшим пропорционально все базовые
			for (auto& res : result) {
				res._percentage *= (100.0f - rarePercent) / 100.0f;
			}

			result.push_back({ rareName, rarePercent });
		}
	}

	return result;
}
