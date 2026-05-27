#include "Game/widgets/code_editor/code_editor_window.h"
#include "CodeEditorWidget/code_editor_widget.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/commands/command_context.h"

#include <QRegularExpression>
#include <QVBoxLayout>
#include <QToolButton>
#include <QFileDialog>

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
	const auto res = QFileDialog::getOpenFileName(this,
		"Выберите файл", "", "All files (*.*)");
	if (!res.isEmpty()) {
		d->path = res;
		onChangeTargetPath();
	}
}

void CodeEditorWindow::onSaveDocumentClick() {
	if (d->path.isEmpty()) {
		const auto res = QFileDialog::getSaveFileName(this,
			"Выберите файл", "", "All files (*.*)");
		if (res.isEmpty()) {
			return;
		}
		
		d->path = res;
	}

	QFile file(d->path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}

	QString content = d->editor->toPlainText();

// TODO:
// сделать корректную обработку endline на уровне загрузки
// сделать настройку
//#ifdef Q_OS_WIN
//	content.replace('\n', "\r\n");
//#endif
// Записываем UTF-8 BOM если нужно
// file.write("\xEF\xBB\xBF"); // раскомментировать если нужен BOM

	QTextStream stream(&file);
	stream.setEncoding(QStringConverter::Utf8);
	stream << content;

	file.close();
}
