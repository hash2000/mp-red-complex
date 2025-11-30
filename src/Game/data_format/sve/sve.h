#pragma once
#include <QString>
#include <map>
#include <vector>
#include <cstdint>

namespace DataFormat::Sve {
// scripted variable entries

struct ScriptEntry {
	uint32_t offset;
	QString text;
};

struct ScriptEntries {
	using ItemsArray = std::vector<ScriptEntry>;
	using Items = std::map<uint32_t, ScriptEntry>;

	Items items;
};

[[nodiscard]] ScriptEntries::ItemsArray to_array(ScriptEntries::Items &items);

}
