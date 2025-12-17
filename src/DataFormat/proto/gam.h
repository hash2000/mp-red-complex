#pragma once
#include <QString>
#include <map>
#include <vector>
#include <memory>
#include <cstdint>

namespace Proto {
// Script global variables

struct GlobalVar {
	QString name;
	QString comment;
	uint32_t index;
	int32_t value;
};

struct GlobalVariables {
	using ItemsArray = std::vector<std::shared_ptr<GlobalVar>>;
	using Items = std::map<QString, std::shared_ptr<GlobalVar>>;

	Items gameVars;
	Items mapVars;
};

} // namespace Proto
