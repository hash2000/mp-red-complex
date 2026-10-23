#include "Libs/Resources/variables/variables_context.h"

QVariant VariablesContext::get(const QString& name, const QVariant& def) const {
	if (!_variables.contains(name)) {
		return QVariant(def);
	}

	return _variables[name];
}

QVariant VariablesContext::set(const QString& name, const QVariant& value) {
	_variables[name] = value;
	return _variables[name];
}

void VariablesContext::clear() {
	_variables.clear();
}

QStringList VariablesContext::available() const {
	return _variables.keys();
}

void VariablesContext::apply(const VariablesContext& values) {
	const auto keys = values.available();
	for (const auto it : keys) {
		_variables[it] = values.get(it, QVariant());
	}
}
