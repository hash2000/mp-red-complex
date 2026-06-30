#include "Launcher/commands/cmd/windows_close_all_cmd.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/instruction.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

bool CloseAllWindowsCommand::execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	auto controller = context->controllers()->windowsController();
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
