#include "Launcher/commands/cmd/windows_list_cmd.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

bool ListWindowsCommand::execute(CommandContext* context, const QStringList& args) {
	Q_UNUSED(args);

	auto controller = context->controllers()->windowsController();
	auto entries = controller->windowEntries();

	if (entries.isEmpty()) {
		context->print("No windows open", "info");
		return true;
	}

	// Получаем активное окно для маркировки
	auto activeEntry = controller->activeWindowEntry();
	QString activeId = activeEntry.second;

	context->print(QString("Open windows (%1):").arg(entries.size()), "system");

	for (const auto& entry : entries) {
		MdiChildWindow* window = entry.first.data();
		const QString& windowId = entry.second;

		if (!window) {
			continue;
		}

		QString marker = (windowId == activeId) ? " ► " : "   ";
		QString title = window->windowTitle().trimmed();
		if (title.isEmpty()) {
			title = "<untitled>";
		}

		QString line = QString("%1[%2] %3 (%4)")
			.arg(marker)
			.arg(windowId)
			.arg(title)
			.arg(window->windowType());

		context->print(line, (windowId == activeId) ? "success" : "info");
	}

	return true;
}
