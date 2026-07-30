#include "Launcher/windows/fetch_api/fetch_api_window.h"
#include "Launcher/app_controller.h"
#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
#include "Content/FetchApiModule/widgets/fetch_queries_tree_widget.h"
#include "Content/FetchApiModule/services/fetch_store_service.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/models/instruction.h"

#include "Libs/Engine/services/services_registry.h"

#include <QRegularExpression>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QFileDialog>

class FetchApiWindow::Private {
public:
	Private(FetchApiWindow* parent) : q(parent) {}
	FetchApiWindow* q;

	QWidget* mainContainer = nullptr;
	FetchApiWidget* widget = nullptr;
	FetchQueriesTreeWidget* tree = nullptr;

	void showToolWidget(QToolButton* button);

	// Команды
	bool applyInstructionCreate(const std::shared_ptr<Instruction> instruction, CommandContext* context);
};

FetchApiWindow::FetchApiWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
}

FetchApiWindow::~FetchApiWindow() = default;

QString FetchApiWindow::help() const {
	return "fetch-api";
}

QString FetchApiWindow::windowTitle() const {
	return "Fetch api";
}

bool FetchApiWindow::Private::applyInstructionCreate(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	auto services = context->services();

	if (mainContainer) {
		mainContainer->deleteLater();
	}

	mainContainer = new QWidget(q);

	auto mainLayout = new QHBoxLayout(mainContainer);
	auto buttonsLayout = new QVBoxLayout();
	buttonsLayout->setSpacing(3);

	auto toolButton = new QToolButton(mainContainer);
	toolButton->setText("▾");
	toolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
	toolButton->setToolTip("Показать сохранённый запросы");
	buttonsLayout->addWidget(toolButton);

	tree = new FetchQueriesTreeWidget(services->get<FetchStoreService>());
	tree->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	tree->setStyleSheet("border: 1px solid #cccccc; border-radius: 5px;");

	widget = new FetchApiWidget(context, mainContainer);

	buttonsLayout->addStretch(1);
	mainLayout->addWidget(widget, 1);
	mainLayout->addLayout(buttonsLayout);
	q->setWidget(mainContainer);

	connect(toolButton, &QToolButton::clicked, q, [this, toolButton]() {
		showToolWidget(toolButton);
	});

	return true;
}

void FetchApiWindow::Private::showToolWidget(QToolButton* button) {
	if (tree->isVisible()) {
		tree->hide();
		return;
	}

	auto size = widget->size();
	size.setWidth(qMax(size.width() / 4, 300));

	QPoint buttonBottomLeft = button->mapToGlobal(QPoint(-size.width(), button->height()));

	tree->resize(size);
	tree->move(buttonBottomLeft);
	tree->show();
	tree->raise();
}

bool FetchApiWindow::handleCommand(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		return false;
	}

	if (action == "create") return d->applyInstructionCreate(instruction, context);

	return false;
}
