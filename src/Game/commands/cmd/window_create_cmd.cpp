#include "Game/commands/cmd/window_create_cmd.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/controllers/windows_controller.h"
#include "Game/app_controller.h"
#include "Game/controllers.h"
#include "Game/windows_builder.h"
#include <QMdiSubWindow>
#include <QUuid>

bool CreateWindowCommand::execute(CommandContext* context, const QStringList& args) {
	auto app = context->applicationController();
	auto controller = app->controllers()->windowsController();
	auto mdiArea = controller->mdiArea();

	if (args.count() < 1) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	const auto target = args.at(0);
	QString id;

	if (args.count() >= 2) {
		id = args.at(1);
	}
	else {
		id = QUuid::createUuid()
			.toString(QUuid::StringFormat::WithoutBraces);
	}

	WindowsBuilder builder(app);
	auto widget = builder.build(target, id, nullptr);
	if (!widget) {
		context->printError(QString("Can't find target window %1")
			.arg(target));
		return false;
	}

	if (!widget->handleCommand("create", args, context)) {
		delete widget;
		return false;
	}

	const auto title = widget->windowTitle();
	const auto sizes = widget->windowDefaultSizes();

	auto subWndow = mdiArea->addSubWindow(widget);
	subWndow->setWindowTitle(title);
	subWndow->setAttribute(Qt::WA_DeleteOnClose, true);
	subWndow->resize(sizes.width(), sizes.height());
	widget->setMdiArea(mdiArea);
	widget->setupMdiArea();
	subWndow->show();

	if (!controller->registerWindow(widget)) {
		delete widget;
		return false;
	}

	context->printSuccess(QString("Window %1 created with title '%2'")
		.arg(target)
		.arg(title));

	return true;
}
