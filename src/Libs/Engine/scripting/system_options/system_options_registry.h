#pragma once
#include "Libs/Engine/scripting/system_options/system_option.h"

#include <QList>
#include <QStringList>
#include <memory>
#include <optional>

class CommandContext;

/// Реестр описаний системных параметров приложения.
///
/// Сами значения хранятся в переменных глобального контекста; реестр знает,
/// какие параметры существуют, их значения по умолчанию и допустимые значения.
/// Новые параметры добавляются registerOption() — интерфейс настроек и
/// значения по умолчанию подхватывают их автоматически.
class SystemOptionsRegistry {
public:
	SystemOptionsRegistry();
	~SystemOptionsRegistry();

	/// Добавляет или обновляет описание параметра (совпадение по name).
	void registerOption(const SystemOption& option);

	/// Описания параметров в порядке регистрации.
	QList<SystemOption> options() const;

	/// Описание параметра по имени переменной; nullopt — параметр не описан.
	std::optional<SystemOption> option(const QString& name) const;

	QStringList optionNames() const;

	/// Значение по умолчанию для имени; невалидный QVariant — параметра нет.
	QVariant defaultValue(const QString& name) const;

	/// Записывает значения по умолчанию в контекст, если переменной ещё нет.
	/// Возвращает количество записанных параметров.
	int applyDefaults(CommandContext& context) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
