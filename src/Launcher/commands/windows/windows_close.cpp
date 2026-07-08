//#include "Launcher/commands/windows/windows_close.h"
//#include "Launcher/controllers.h"
//#include "Launcher/controllers/windows_controller.h"
//#include "Content/ConsoleModule/command_context.h"
//
//namespace WindowsNs {
//
//bool handleCloseAll(CommandContext* context) {
//	auto controller = context->controllers()->windowsController();
//	auto windows = controller->allWindows();
//	int count = windows.size();
//	for (MdiChildWindow* window : windows) {
//		window->deleteLater();
//	}
//
//	context->printSuccess(QString("Closed %1 window(s)")
//		.arg(count));
//
//	return true;
//}
//
//bool handleCloseActive(CommandContext* context) {
//	auto controller = context->controllers()->windowsController();
//	auto activeEntry = controller->activeWindowEntry();
//	if (!activeEntry.first) {
//		context->printError("No active window");
//		return false;
//	}
//
//	const auto windowId = activeEntry.second;
//	controller->closeWindowById(windowId);
//	context->printSuccess(QString("Closed active window [%1]")
//		.arg(windowId));
//
//	return true;
//}
//
//bool handleCloseById(const QString& id, CommandContext* context) {
//	auto controller = context->controllers()->windowsController();
//	if (controller->findWindowById(id)) {
//		controller->closeWindowById(id);
//		context->printSuccess(QString("Closed window [%1]")
//			.arg(id));
//		return true;
//	}
//
//	context->printError(QString("Window not found: %1")
//		.arg(id));
//
//	return false;
//}
//
//bool handleClose(const QString& id, CommandContext* context) {
//	if (id == "all") return handleCloseAll(context);
//	else if (id == "active") return handleCloseActive(context);
//
//	return handleCloseById(id, context);
//}
//}
