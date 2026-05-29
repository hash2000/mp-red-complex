#include "Game/commands/cmd/window_create_cmd.h"
#include "Game/commands/command_context.h"
#include "Game/controllers/windows_controller.h"
#include "Game/app_controller.h"
#include "Game/controllers.h"
#include "Game/windows_builder.h"

#include <QMdiSubWindow>
#include <QRegularExpression>
#include <QUuid>

bool CreateWindowCommand::execute(CommandContext* context, const QStringList& args) {
	auto app = context->applicationController();
	auto controller = context->controllers()->windowsController();
	auto mdiArea = controller->mdiArea();

	if (args.count() < 1) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	auto id = args.filter(QRegularExpression("^id:")).value(0).mid(3);
	const auto target = args.filter(QRegularExpression("^target:")).value(0).mid(7);
	const auto alternateTitle = args.filter(QRegularExpression("^title:")).value(0).mid(6);

	if (target.isEmpty()) {
		context->printError(QString("Need parameter 'target'. Usage: %1").arg(help()));
		return false;
	}

	if (id.isEmpty()){
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
		context->printError(QString("Method create returned false. %1")
			.arg(target));
		delete widget;
		return false;
	}

	const auto title = widget->windowTitle();
	const auto sizes = widget->windowDefaultSizes();
	auto subWndow = mdiArea->addSubWindow(widget);

	if (alternateTitle.isEmpty()) {
		subWndow->setWindowTitle(title);
	}
	else {
		subWndow->setWindowTitle(alternateTitle);
	}

	subWndow->setAttribute(Qt::WA_DeleteOnClose, true);
	subWndow->resize(sizes.width(), sizes.height());
	widget->setMdiArea(mdiArea);
	widget->setupMdiArea();
	subWndow->show();

	if (!controller->registerWindow(widget)) {
		context->printError(QString("Method create returned false. target '%1' title '%2' and id '%3'")
			.arg(target)
			.arg(title)
			.arg(id));
		delete widget;
		return false;
	}

	context->printSuccess(QString("Can't register window '%1' with title '%2' and id '%3'")
		.arg(target)
		.arg(title)
		.arg(id));

	return true;
}
