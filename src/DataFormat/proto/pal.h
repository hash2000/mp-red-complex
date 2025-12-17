#pragma once
#include <QRgb>
#include <vector>

namespace Proto {
	struct Pallete {
		using Items = std::vector<QRgb>;

		Items items;
	};
}
