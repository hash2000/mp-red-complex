#include "Launcher/windows/fetch_api/fetch_api_queries_tree_window.h"
#include "Content/FetchApiModule/widgets/fetch_queries_tree_widget.h"
#include "Content/FetchApiModule/services/fetch_store_db_service.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/models/instruction.h"

#include "Libs/Engine/services/services_registry.h"

#include "Launcher/app_controller.h"

#include <QRegularExpression>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QFileDialog>

class FetchApiQueriesTreeWindow::Private {
public:
	Private(FetchApiQueriesTreeWindow* parent) : q(parent) {}
	FetchApiQueriesTreeWindow* q;

	FetchQueriesTreeWidget* tree = nullptr;

	bool applyInstructionCreate(const std::shared_ptr<Instruction> instruction);
};

FetchApiQueriesTreeWindow::FetchApiQueriesTreeWindow(const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent)
	: MdiChildWindow(id, context, parent)
	, d(std::make_unique<Private>(this)) {}

FetchApiQueriesTreeWindow::~FetchApiQueriesTreeWindow() = default;

QString FetchApiQueriesTreeWindow::help() const {
	return "fetch-api-queries-tree";
}

QString FetchApiQueriesTreeWindow::windowTitle() const {
	return "fetch-api-queries-tree";
}

bool FetchApiQueriesTreeWindow::handleCommand(const std::shared_ptr<Instruction> instruction) {
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		return false;
	}

	if (action == "create") {
		return d->applyInstructionCreate(instruction);
	}

	return false;
}

bool FetchApiQueriesTreeWindow::Private::applyInstructionCreate(const std::shared_ptr<Instruction> instruction) {
	const auto context = q->currentContext();
	const auto services = context->commandController()->services();

	if (tree) {
		tree->deleteLater();
	}

	tree = new FetchQueriesTreeWidget(services->get<FetchStoreDbService>());
	tree->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	tree->setStyleSheet("border: 1px solid #cccccc; border-radius: 5px;");
	q->setWidget(tree);

	return true;
}
