#pragma once
#include <memory>

class CommandController;

/// Регистрирует нативные классы виджетов приложения в WidgetRegistry.
/// Дальше эти классы можно использовать в yaml-описании приложения
/// (applications/{name}/main.yaml) по полю xtype или по полному имени класса.
class WidgetsBuilder {
public:
	explicit WidgetsBuilder(CommandController* commandController);
	~WidgetsBuilder();

private:
	class Private;
	std::unique_ptr<Private> d;
};
