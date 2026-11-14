#include "Launcher/commands/windows/windows_list.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/console/console_image.h"
#include "Libs/Engine/services/services_registry.h"

#include "Libs/Engine/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

namespace WindowsNs {
bool handleList(std::shared_ptr<CommandContext> context, Controllers* controllers) {
	const auto wndController = controllers->windowsController();
	const auto cmdController = context->commandController();
	auto entries = wndController->windowEntries();

	if (entries.isEmpty()) {
		cmdController->print("No windows open", "info");
		return true;
	}

	auto activeEntry = wndController->activeWindowEntry();
	QString activeId = activeEntry.second;

	cmdController->print(QString("Open windows (%1):")
		.arg(entries.size()), "system");

	ConsoleTable table({ "..", "Id", "Title", "Type", "Status" });

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

		table.addRow({
			marker,
			windowId,
			title,
			window->windowType(),
			(windowId == activeId) ? "success" : "info"
			});

		cmdController->print(table);
	}

	return true;
}
}
