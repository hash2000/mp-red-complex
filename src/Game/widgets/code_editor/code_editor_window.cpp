#include "Game/widgets/code_editor/code_editor_window.h"
#include "CodeEditorWidget/code_editor_widget.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/commands/command_context.h"
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QToolButton>

class CodeEditorWindow::Private {
public:
	Private(CodeEditorWindow* parent) : q(parent) {}
	CodeEditorWindow* q;

	CommandContext* context = nullptr;
	CodeEditorWidget* editor = nullptr;
	QWidget* buttonsContainer = nullptr;
	QVBoxLayout* buttonsLayout = nullptr;
	QString path;

	void setupUI(Services* services);
	void setupButtons();
	QToolButton* addButton(const QString& title, const QString& tooltip);
};

CodeEditorWindow::CodeEditorWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
}

CodeEditorWindow::~CodeEditorWindow() = default;

QToolButton* CodeEditorWindow::Private::addButton(const QString& title, const QString& tooltip) {
	auto btn = new QToolButton(buttonsContainer);
	buttonsLayout->addWidget(btn);
	btn->setText(title);
	btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
	btn->setToolTip(tooltip);
	return btn;
}

void CodeEditorWindow::Private::setupButtons() {
	connect(addButton("📥", "Загрузить"), &QToolButton::clicked, q, &CodeEditorWindow::onOpenDocumentClick);
	connect(addButton("💾", "Сохранить"), &QToolButton::clicked, q, &CodeEditorWindow::onSaveDocumentClick);
}

void CodeEditorWindow::Private::setupUI(Services* services) {
	buttonsContainer = new QWidget(q);
	auto mainLayout = new QHBoxLayout(buttonsContainer);
	mainLayout->setContentsMargins(2, 2, 2, 2);
	mainLayout->setSpacing(2);

	buttonsLayout = new QVBoxLayout();
	buttonsLayout->setSpacing(2);
	setupButtons();
	buttonsLayout->addStretch();

	editor = new CodeEditorWidget(services->highlightingPluginManager(), buttonsContainer);
	mainLayout->addWidget(editor, 1);
	mainLayout->addLayout(buttonsLayout);

	q->setWidget(buttonsContainer);
}

bool CodeEditorWindow::handleCommand(const QString& commandName,
	const QStringList& args,
	CommandContext* context) {
	if (commandName == "create") {
		auto controller = context->applicationController();
		auto services = controller->services();

		d->context = context;
		d->path = args.filter(QRegularExpression("^path:")).value(0).mid(5);

		d->setupUI(services);
		//setWidget(d->widget);

		if (!d->path.isEmpty()) {
			onChangeTargetPath();
		}

		return true;
	}

	return false;
}

void CodeEditorWindow::onChangeTargetPath() {
	if (d->path.isEmpty()) {
		d->context->printError(QString("Usage: [required window-create parameters] %1").arg(help()));
		return;
	}

	d->editor->setPath(d->path);
}

void CodeEditorWindow::onOpenDocumentClick() {

}

void CodeEditorWindow::onSaveDocumentClick() {

}
