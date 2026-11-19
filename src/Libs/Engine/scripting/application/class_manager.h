#pragma once
#include <memory>

class Resources;
class CommandController;
class QWidget;

namespace BootstrapConfig {
struct Application;
}

/// Загружает yaml-описание приложения (applications/{name}/main.yaml),
/// разрешает наследование классов виджетов и создаёт вьюпорт через WidgetRegistry.
class ClassManager {
public:
	ClassManager(Resources* resources, CommandController* commandController);
	virtual ~ClassManager();

	/// Создаёт виджет вьюпорта (обычно MainWindow) по описанию приложения.
	/// Возвращает nullptr, если описание недоступно или вьюпорт не зарегистрирован.
	QWidget* createViewport(const BootstrapConfig::Application& application);

private:
	class Private;
	std::unique_ptr<Private> d;
};
