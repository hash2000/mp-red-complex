#include "Resources/variables/variables_context.h"

QVariant VariablesContext::get(const QString& name, const QString& def) {
	if (!_variables.contains(name)) {
		return QVariant(def);
	}

	return _variables[name];
}

QVariant VariablesContext::set(const QString& name, const QString& value) {
	_variables[name] = value;
	return _variables[name];
}
