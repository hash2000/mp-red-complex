#include "Launcher/commands/windows/windows_list.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_image.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Engine/services/services_registry.h"

#include "Libs/Engine/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

namespace WindowsNs {
bool handleList(CommandContext* context, Controllers* controllers) {
	auto controller = controllers->windowsController();
	auto entries = controller->windowEntries();

	if (entries.isEmpty()) {
		context->print("No windows open", "info");
		return true;
	}

	auto activeEntry = controller->activeWindowEntry();
	QString activeId = activeEntry.second;

	context->print(QString("Open windows (%1):")
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

		context->print(table);
	}

	return true;
}
}
