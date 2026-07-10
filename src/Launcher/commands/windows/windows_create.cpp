#include "Launcher/commands/windows/windows_create.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_image.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Engine/services/services_registry.h"

#include "Libs/Engine/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/windows_builder.h"

#include <QUuid>

namespace WindowsNs {
bool handleCreate(const std::shared_ptr<Instruction> instruction,
	CommandContext* context,
	Controllers* controllers) {
	auto controller = controllers->windowsController();
	auto mdiArea = controller->mdiArea();
	auto id = instruction->text("id");
	const auto target = instruction->text("target");
	const auto alternateTitle = instruction->text("title");

	if (id.isEmpty()) {
		id = QUuid::createUuid()
			.toString(QUuid::StringFormat::WithoutBraces);
	}

	if (controller->checkWindowRegistration(id)) {
		qWarning() << "Window allready registered" << id;
		return false;
	}

	WindowsBuilder builder;
	auto widget = builder.build(target, id, nullptr);
	if (!widget) {
		context->printError(QString("Can't find target window %1")
			.arg(target));
		return false;
	}

	if (!widget->handleCommand(instruction, context)) {
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
//	subWndow->resize(sizes.width(), sizes.height());
	widget->setMdiArea(mdiArea);
	widget->setupMdiArea();
	subWndow->show();

	if (!controller->registerWindow(widget)) {
		context->printError(QString("Method create returned false. target '%1' title '%2' and id '%3'")
			.arg(target)
			.arg(title)
			.arg(id));
		widget->deleteLater();
		return false;
	}

	context->printSuccess(QString("Register window '%1' with title '%2' and id '%3'")
		.arg(target)
		.arg(title)
		.arg(id));

	return true;
}
}
