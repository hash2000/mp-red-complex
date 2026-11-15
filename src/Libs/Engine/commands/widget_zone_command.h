#pragma once
#include "Libs/Engine/scripting/i_command.h"
#include "Libs/Engine/ui/main_window.h"

/// Управляет одной областью главного окна: документами, нижними
/// информационными панелями или боковыми панелями.
///
/// Идёт в Engine, потому что работает только с MainWindow и WidgetRegistry
/// и не знает о конкретном приложении-хосте.
class WidgetZoneCommand : public ICommand {
public:
	WidgetZoneCommand(const QString& name, MainWindow::WidgetKind kind, QObject* parent = nullptr);

	QString name() const override;
	QString description() const override;
	QString help() const override;

	bool execute(std::shared_ptr<CommandContext> context) override;

private:
	bool executeCreate(const std::shared_ptr<CommandContext>& context);
	bool executeClose(const std::shared_ptr<CommandContext>& context);
	bool executeShow(const std::shared_ptr<CommandContext>& context);
	bool executeList(const std::shared_ptr<CommandContext>& context);

	QString _name;
	MainWindow::WidgetKind _kind;
};
