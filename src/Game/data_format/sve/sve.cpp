#include "Game/data_format/sve/sve.h"

namespace DataFormat::Sve {

	ScriptEntries::ItemsArray to_array(ScriptEntries::Items &items) {
		ScriptEntries::ItemsArray result;

		const auto size = items.size();
		size_t i = 0;

		result.resize(size);

		for (const auto &it : items) {
			result[i++] = it.second;
		}

		return result;
	}

}
