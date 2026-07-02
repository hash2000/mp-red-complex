#include "Launcher/widgets/code_editor/code_editor_window.h"
#include "Content/CodeEditorWidget/code_editor_widget.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/instruction.h"

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
	QString documentPath;

	void setupUI(Services* services);
	void setupButtons();
	QToolButton* addButton(const QString& title, const QString& tooltip);
	void changeTargetPath(const QString& path);

	// Команды
	bool applyInstructionCreate(const std::shared_ptr<Instruction> instruction, CommandContext* context);
	bool applyInstructionStyle(const std::shared_ptr<Instruction> instruction, CommandContext* context);
	bool applyInstructionLanguageHighlighter(const std::shared_ptr<Instruction> instruction, CommandContext* context);
	bool applyInstructionPlantText(const std::shared_ptr<Instruction> instruction, CommandContext* context);
	bool applyInstructionFormatDocument(const std::shared_ptr<Instruction> instruction, CommandContext* context);
	bool handleStyleWordWrap(const QString& value);
	bool handleStyleFont(const QString& value);
};

CodeEditorWindow::CodeEditorWindow(const QString& id, QWidget* parent)
	: MdiChildWindow(id, parent)
	, d(std::make_unique<Private>(this)) {
}

CodeEditorWindow::~CodeEditorWindow() = default;

QString CodeEditorWindow::help() const {
	return R"(code-editor action:
		create path:<path>
		style word-wrap:[true|false] font:[fontname]
)";
}

QString CodeEditorWindow::windowTitle() const {
	if (!d->documentPath.isEmpty()) {
		return d->documentPath;
	}

	return "Editor";
}

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

	editor = new CodeEditorWidget(
		services->highlightingPluginManager(),
		services->formatterPluginManager(),
		buttonsContainer);
	editor->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
	editor->setLineWrapMode(QTextEdit::LineWrapMode::NoWrap);
	mainLayout->addWidget(editor, 1);
	mainLayout->addLayout(buttonsLayout);

	q->setWidget(buttonsContainer);
}

bool CodeEditorWindow::handleCommand(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		return false;
	}

	if (action == "create") return d->applyInstructionCreate(instruction, context);
	else if (action == "style") return d->applyInstructionStyle(instruction, context);

	return false;
}

bool CodeEditorWindow::Private::handleStyleWordWrap(const QString& value) {
	if (value.isEmpty()) {
		return false;
	}

	if (value == "true") {
		this->editor->setLineWrapMode(QTextEdit::LineWrapMode::WidgetWidth);
	}
	else {
		this->editor->setLineWrapMode(QTextEdit::LineWrapMode::NoWrap);
	}

	return true;
}

bool CodeEditorWindow::Private::handleStyleFont(const QString& value) {
	if (value.isEmpty()) {
		return false;
	}

	return true;
}

bool CodeEditorWindow::Private::applyInstructionLanguageHighlighter(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	return this->editor->setLanguage(instruction->text("lang", "txt"));
}

bool CodeEditorWindow::Private::applyInstructionPlantText(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	this->editor->setText(instruction->text("text"));
	return true;
}

bool CodeEditorWindow::Private::applyInstructionFormatDocument(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	if (instruction->text("format-document") == "true") {
		this->editor->formatDocument();
	}
	return true;
}

bool CodeEditorWindow::Private::applyInstructionStyle(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	if (!this->editor) {
		return false;
	}

	handleStyleWordWrap(instruction->text("word-wrap"));
	handleStyleFont(instruction->text("font"));

	return true;
}

bool CodeEditorWindow::Private::applyInstructionCreate(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	auto services = context->services();
	this->context = context;
	const auto path = instruction->text("path");
	this->setupUI(services);
	changeTargetPath(path);

	applyInstructionStyle(instruction, context);
	applyInstructionLanguageHighlighter(instruction, context);
	applyInstructionPlantText(instruction, context);
	applyInstructionFormatDocument(instruction, context);

	return true;
}

void CodeEditorWindow::Private::changeTargetPath(const QString& path) {
	if (path.isEmpty()) {
		return;
	}

	documentPath = path;

	editor->setPath(documentPath);
	q->setWindowTitle(documentPath);
}

void CodeEditorWindow::formatDocument() {
	d->editor->formatDocument();
}

void CodeEditorWindow::onOpenDocumentClick() {
	const auto res = QFileDialog::getOpenFileName(this,
		"Выберите файл", "", "All files (*.*)");
	if (!res.isEmpty()) {
		d->changeTargetPath(res);
	}
}

void CodeEditorWindow::onSaveDocumentClick() {
	if (d->documentPath.isEmpty()) {
		const auto res = QFileDialog::getSaveFileName(this,
			"Выберите файл", "", "All files (*.*)");
		if (res.isEmpty()) {
			return;
		}
		
		d->changeTargetPath(res);
	}

	QFile file(d->documentPath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}

	QString content = d->editor->toPlainText();
	QTextStream stream(&file);
	stream.setEncoding(QStringConverter::Utf8);
	stream << content;

	file.close();
}
