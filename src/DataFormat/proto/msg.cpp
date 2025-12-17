#include "DataFormat/proto/msg.h"

namespace Proto {

	Messages::ItemsArray to_array(Messages::Items &items) {
		Messages::ItemsArray result;

		const auto size = items.size();
		size_t i = 0;

		result.resize(size);

		for (const auto &it : items) {
			result[i++] = it.second;
		}

		return result;
	}

}
