#include "Game/commands/cmd/window_create_cmd.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/controllers/windows_controller.h"
#include "Game/app_controller.h"
#include "Game/windows_builder.h"
#include <QMdiSubWindow>

bool CreateWindowCommand::execute(CommandContext* context, const QStringList& args) {
	auto app = context->applicationController();
	auto controller = app->windowsController();
	auto mdiArea = controller->mdiArea();

	if (args.isEmpty()) {
		context->printError("Usage: window-create <type>");
		return false;
	}

	const auto target = args.first();

	WindowsBuilder builder(app);
	auto widget = builder.build(target);
	if (!widget) {
		context->printError(QString("Can't find target window %1")
			.arg(target));
		return false;
	}

	auto subWndow = mdiArea->addSubWindow(widget);
	const auto title = subWndow->windowTitle();
	controller->registerWindow(widget);
	subWndow->setWindowTitle(title);
	subWndow->setAttribute(Qt::WA_DeleteOnClose, true);
	subWndow->resize(640, 480);
	subWndow->show();

	context->printSuccess(QString("Window %1 created with title '%2'")
		.arg(target)
		.arg(title));

	return true;
}
