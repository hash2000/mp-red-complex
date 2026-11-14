#include "Launcher/commands/windows/windows_create.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/console/console_image.h"
#include "Libs/Engine/services/services_registry.h"

#include "Libs/Engine/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/windows_builder.h"

#include <QUuid>

namespace WindowsNs {
bool handleCreate(std::shared_ptr<CommandContext> context, Controllers* controllers) {
	const auto wndController = controllers->windowsController();
	const auto cmdController = context->commandController();
	auto mdiArea = wndController->mdiArea();
	auto id = context->variable("id").toString();
	const auto target = context->variable("target").toString();
	const auto alternateTitle = context->variable("title").toString();

	if (id.isEmpty()) {
		id = QUuid::createUuid()
			.toString(QUuid::StringFormat::WithoutBraces);
	}

	if (wndController->checkWindowRegistration(id)) {
		qWarning() << "Window allready registered" << id;
		return false;
	}

	WindowsBuilder builder;
	// Окно получает контекст вызывающей стороны: контекст команды живёт только на время вызова.
	auto widget = builder.build(target, id, context->parentContext(), nullptr);
	if (!widget) {
		cmdController->printError(QString("Can't find target window %1")
			.arg(target));
		return false;
	}

	if (!widget->handleCommand(context)) {
		cmdController->printError(QString("Method create returned false. %1")
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
//	subWndow->resize(sizes.width(), sizes.height());
	widget->setMdiArea(mdiArea);
	widget->setupMdiArea();
	subWndow->show();

	if (!wndController->registerWindow(widget)) {
		cmdController->printError(QString("Method create returned false. target '%1' title '%2' and id '%3'")
			.arg(target)
			.arg(title)
			.arg(id));
		widget->deleteLater();
		return false;
	}

	cmdController->printSuccess(QString("Register window '%1' with title '%2' and id '%3'")
		.arg(target)
		.arg(title)
		.arg(id));

	return true;
}
}
