#include "Launcher/windows/code_editor/code_editor_window.h"
#include "Content/CodeEditorWidget/code_editor_widget.h"
#include "Launcher/app_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"

#include "Libs/Engine/services/services_registry.h"


#include <QRegularExpression>
#include <QVBoxLayout>
#include <QToolButton>
#include <QFileDialog>

class CodeEditorWindow::Private {
public:
  Private(CodeEditorWindow* parent) : q(parent) {}
  CodeEditorWindow* q;

  CodeEditorWidget* editor = nullptr;
  QWidget* buttonsContainer = nullptr;
  QVBoxLayout* buttonsLayout = nullptr;
  QString documentPath;

  void setupUI(ServicesRegistry* services);
  void setupButtons();
  QToolButton* addButton(const QString& title, const QString& tooltip);
  void changeTargetPath(const QString& path);

  // Команды
  bool applyCommandCreate(std::shared_ptr<CommandContext> context);
  bool applyCommandStyle(std::shared_ptr<CommandContext> context);
  bool applyCommandLanguageHighlighter(std::shared_ptr<CommandContext> context);
  bool applyCommandContentTypeHighlighter(std::shared_ptr<CommandContext> context);
  bool applyCommandPlantText(std::shared_ptr<CommandContext> context);
  bool applyCommandFormatDocument(std::shared_ptr<CommandContext> context);
  bool handleStyleWordWrap(const QString& value);
  bool handleStyleFont(const QString& value);
};

CodeEditorWindow::CodeEditorWindow(const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent)
  : MdiChildWindow(id, context, parent)
  , d(std::make_unique<Private>(this)) {
}

CodeEditorWindow::~CodeEditorWindow() = default;

QString CodeEditorWindow::help() const {
  return R"(code-editor action=<command>[, param=<value>]
    create path=<path>
    style word-wrap=<true|false>, font=<fontname>
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
  connect(addButton("💾", "Сохранить"), &QToolButton::clicked, q, &CodeEditorWindow::onSaveDocumentClick);
  connect(addButton("📥", "Загрузить"), &QToolButton::clicked, q, &CodeEditorWindow::onOpenDocumentClick);
}

void CodeEditorWindow::Private::setupUI(ServicesRegistry* services) {
  buttonsContainer = new QWidget(q);
  auto mainLayout = new QHBoxLayout(buttonsContainer);
  mainLayout->setContentsMargins(2, 2, 2, 2);
  mainLayout->setSpacing(2);

  buttonsLayout = new QVBoxLayout();
  buttonsLayout->setSpacing(2);
  setupButtons();
  buttonsLayout->addStretch();

  editor = new CodeEditorWidget(
    services->get<HighlightingPluginManager>(),
    services->get<FormatterPluginManager>(),
    buttonsContainer);
  editor->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
  editor->setLineWrapMode(QTextEdit::LineWrapMode::NoWrap);
  mainLayout->addWidget(editor, 1);
  mainLayout->addLayout(buttonsLayout);

  q->setWidget(buttonsContainer);
}

bool CodeEditorWindow::handleCommand(std::shared_ptr<CommandContext> context) {
  const auto action = context->variable("action").toString();
  if (action.isEmpty()) {
    return false;
  }

  if (action == "create") {
    return d->applyCommandCreate(context);
  }
  else if (action == "style") {
    return d->applyCommandStyle(context);
  }

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

bool CodeEditorWindow::Private::applyCommandContentTypeHighlighter(std::shared_ptr<CommandContext> context) {
  return this->editor->setContentType(context->variable("content-type", QString("text/plane")).toString());
}

bool CodeEditorWindow::Private::applyCommandLanguageHighlighter(std::shared_ptr<CommandContext> context) {
  return this->editor->setLanguage(context->variable("lang", QString("txt")).toString());
}

bool CodeEditorWindow::Private::applyCommandPlantText(std::shared_ptr<CommandContext> context) {
  this->editor->setText(context->variable("text").toString());
  return true;
}

bool CodeEditorWindow::Private::applyCommandFormatDocument(std::shared_ptr<CommandContext> context) {
  if (context->variable("format-document").toString() == "true") {
    this->editor->formatDocument();
  }
  return true;
}

bool CodeEditorWindow::Private::applyCommandStyle(std::shared_ptr<CommandContext> context) {
  if (!this->editor) {
    return false;
  }

  handleStyleWordWrap(context->variable("word-wrap").toString());
  handleStyleFont(context->variable("font").toString());

  return true;
}

bool CodeEditorWindow::Private::applyCommandCreate(std::shared_ptr<CommandContext> context) {
  const auto windowContext = q->currentContext();
  const auto services = windowContext->commandController()->services();
  const auto path = context->variable("path").toString();
  setupUI(services);
  changeTargetPath(path);

  if (!path.isEmpty()) {
    editor->openFile(path);
  }

  applyCommandStyle(context);
  applyCommandLanguageHighlighter(context);
  applyCommandContentTypeHighlighter(context);
  applyCommandPlantText(context);
  applyCommandFormatDocument(context);

  return true;
}

void CodeEditorWindow::Private::changeTargetPath(const QString& path) {
  if (path.isEmpty()) {
    return;
  }

  documentPath = path;
  q->setWindowTitle(documentPath);
}

void CodeEditorWindow::formatDocument() {
  d->editor->formatDocument();
}

void CodeEditorWindow::onOpenDocumentClick() {
  const auto res = QFileDialog::getOpenFileName(this,
    "Открыть файл", "", "All files (*.*)");
  if (!res.isEmpty()) {
    d->changeTargetPath(res);
    d->editor->openFile(res);
  }
}

void CodeEditorWindow::onSaveDocumentClick() {
  if (d->documentPath.isEmpty()) {
    const auto res = QFileDialog::getSaveFileName(this,
      "Сохранить файл", "", "All files (*.*)");
    if (res.isEmpty()) {
      return;
    }
    
    d->changeTargetPath(res);
    d->editor->setHoghlighterByPath(res);
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
