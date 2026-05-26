#include "Game/widgets/code_editor/code_editor_window.h"
#include "Game/widgets/code_editor/code_editor_widget.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/commands/command_context.h"
#include <QRegularExpression>
#include <QVBoxLayout>

class CodeEditorWindow::Private {
public:
	Private(CodeEditorWindow* parent) : q(parent) {}
	CodeEditorWindow* q;

	CommandContext* context = nullptr;
	CodeEditorWidget* widget = nullptr;
	QString path;
};

CodeEditorWindow::CodeEditorWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
}

CodeEditorWindow::~CodeEditorWindow() = default;

bool CodeEditorWindow::handleCommand(const QString& commandName,
	const QStringList& args,
	CommandContext* context) {
	if (commandName == "create") {
		auto controller = context->applicationController();
		auto services = controller->services();
		auto mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(4, 8, 4, 8);
		mainLayout->setSpacing(6);

		d->context = context;
		d->path = args.filter(QRegularExpression("^path:")).value(0).mid(5);
		d->widget = new CodeEditorWidget(this);


		mainLayout->addWidget(d->widget);
		mainLayout->addStretch();
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

	d->widget->setPath(d->path);
}
