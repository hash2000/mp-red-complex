//#include "Launcher/commands/windows.h"
//#include "Launcher/controllers/windows_controller.h"
//#include "Launcher/app_controller.h"
//#include "Launcher/controllers.h"
//#include "Launcher/services.h"
//#include "Launcher/commands/windows/windows_close.h"
//#include "Launcher/commands/windows/windows_list.h"
//#include "Launcher/commands/windows/windows_create.h"
//#include "Launcher/commands/windows/windows_invoke.h"
//
//#include "Content/ConsoleModule/processors/command_processor.h"
//#include "Content/ConsoleModule/command_context.h"
//#include "Content/ConsoleModule/command_console/console_table.h"
//#include "Content/ConsoleModule/command_console/console_image.h"
//#include "Content/ConsoleModule/models/instruction.h"
//
//class WindowsCommand::Private {
//public:
//	Private(WindowsCommand* parent) : q(parent) {}
//	WindowsCommand* q;
//};
//
//WindowsCommand::WindowsCommand(QObject* parent)
//	: d(std::make_unique<Private>(this))
//	, ICommand(parent) {}
//
//WindowsCommand::~WindowsCommand() = default;
//
//QString WindowsCommand::help() const {
//	return R"(windows action:
//	close id:{id | active | all}
//	list
//	create target:{window type} id:{id}(optional) title:{title}(optional)
//	invoke id:{id | active} cmd:{window command}
//		Example:
//		window action:invoke cmd:"style word-wrap:true font:Sans"
//	)";
//}
//
//bool WindowsCommand::execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
//	const auto action = instruction->text("action");
//	if (action.isEmpty()) {
//		context->printError(QString("Usage: %1").arg(help()));
//		return false;
//	}
//
//	if (action == "close") return WindowsNs::handleClose(instruction->text("id"), context);
//	else if (action == "create") return WindowsNs::handleCreate(instruction, context);
//	else if (action == "list") return WindowsNs::handleList(context);
//	else if (action == "invoke") return WindowsNs::handleInvoke(instruction, context);
//
//	context->printError(QString("Usage: %1").arg(help()));
//	return false;
//}
