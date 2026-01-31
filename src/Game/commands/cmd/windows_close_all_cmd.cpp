#include "Game/commands/cmd/windows_close_all_cmd.h"
#include "Game/commands/command_context.h"
#include "Game/app_controller.h"

bool CloseAllWindowsCommand::execute(CommandContext* context, const QStringList& args) {
	Q_UNUSED(args);

	auto controller = context->applicationController();
	auto windows = controller->allWindows();

	int count = windows.size();
	for (MdiChildWindow* window : windows) {
		// Используем deleteLater для безопасного удаления в правильном потоке
		window->deleteLater();
	}

	context->printSuccess(QString("Closed %1 window(s)")
		.arg(count));

	return true;
}
