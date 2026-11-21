#include "Libs/Engine/scripting/system_options/system_options_registry.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QDebug>

class SystemOptionsRegistry::Private {
public:
	QList<SystemOption> options;
};

SystemOptionsRegistry::SystemOptionsRegistry()
	: d(std::make_unique<Private>()) {
}

SystemOptionsRegistry::~SystemOptionsRegistry() = default;

void SystemOptionsRegistry::registerOption(const SystemOption& option) {
	if (option.name.isEmpty()) {
		qWarning() << "SystemOptionsRegistry: empty option name";
		return;
	}

	// Совпадение по имени обновляет описание: так модуль может дополнить
	// параметр (например, расширить список допустимых значений).
	for (auto& existing : d->options) {
		if (existing.name != option.name) {
			continue;
		}

		existing = option;
		return;
	}

	d->options.append(option);
}

QList<SystemOption> SystemOptionsRegistry::options() const {
	return d->options;
}

std::optional<SystemOption> SystemOptionsRegistry::option(const QString& name) const {
	for (const auto& option : d->options) {
		if (option.name == name) {
			return option;
		}
	}

	return std::nullopt;
}

QStringList SystemOptionsRegistry::optionNames() const {
	QStringList names;
	names.reserve(d->options.size());
	for (const auto& option : d->options) {
		names << option.name;
	}

	return names;
}

QVariant SystemOptionsRegistry::defaultValue(const QString& name) const {
	const auto found = option(name);
	return found ? found->defaultValue : QVariant();
}

int SystemOptionsRegistry::applyDefaults(CommandContext& context) const {
	int applied = 0;
	for (const auto& option : d->options) {
		// Уже заданное значение (в том числе изменённое пользователем) не трогаем.
		if (context.hasVariable(option.name)) {
			continue;
		}

		if (context.setVariable(option.name, option.defaultValue)) {
			applied++;
		}
	}

	return applied;
}
