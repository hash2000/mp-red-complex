#pragma once
#include <QVariant>
#include <vector>

namespace Proto {
	struct ScriptContext {
		std::vector<QVariant> returns;
		std::vector<QVariant> data;
		size_t dvar_base = 0;
		size_t svar_base = 0;
	};
} // namespace Proto
