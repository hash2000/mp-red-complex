#include "Launcher/windows/fetch_api/fetch_api_window.h"
#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
#include "Launcher/app_controller.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/models/instruction.h"

#include "Libs/Engine/services/services_registry.h"

#include <QRegularExpression>
#include <QVBoxLayout>
#include <QToolButton>
#include <QFileDialog>

class FetchApiWindow::Private {
public:
	Private(FetchApiWindow* parent) : q(parent) {}
	FetchApiWindow* q;

	FetchApiWidget* widget = nullptr;

	void setupUI(ServicesRegistry* services);

	// Команды
	bool applyInstructionCreate(const std::shared_ptr<Instruction> instruction, CommandContext* context);
};

FetchApiWindow::FetchApiWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {}

FetchApiWindow::~FetchApiWindow() = default;

QString FetchApiWindow::help() const {
	return R"(fetch-api)";
}

QString FetchApiWindow::windowTitle() const {
	return "Fetch api";
}

bool FetchApiWindow::Private::applyInstructionCreate(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	if (widget) {
		widget->deleteLater();
	}

	widget = new FetchApiWidget(q);
	q->setWidget(widget);
	return true;
}

bool FetchApiWindow::handleCommand(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		return false;
	}

	if (action == "create") return d->applyInstructionCreate(instruction, context);

	return false;
}
