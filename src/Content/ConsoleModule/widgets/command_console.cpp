#include "Content/ConsoleModule/widgets/command_console.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Libs/Base/extensions/text_edit_extensions.h"

#include <QFontDatabase>
#include <QScrollBar>
#include <QKeyEvent>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QCompleter>
#include <QStyledItemDelegate>
#include <QListWidget>
#include <QToolButton>
#include <QStringListModel>
#include <QSplitter>

namespace {
// Настройка стилей окна вывода
static QString kQTextEditOutputAreaStyle = R"(
    QTextEdit {
        background-color: #1e1e1e;
        color: #d4d4d4;
        border: 1px solid #3e3e42;
        border-radius: 3px;
        font-family: 'Consolas', 'Courier New', monospace;
        font-size: 10pt;
    }
)";
// Настройка форматирования текста
static QString kQTextEditOutputAreaDocumentStyleSheet = R"(		
    .timestamp { color: #808080; font-size: 8pt; }
    .command { color: #569cd6; font-weight: bold; }
    .error { color: #f44336; }
    .success { color: #66cc66; }
    .system { color: #79b8ff; font-style: italic; }
    .info { color: #d4d4d4; }
		.warning { color: #FFE66D; }

		/* Стили для таблиц */
    table.console-table {
        border-collapse: collapse;
        margin: 8px 0;
        width: 100%;
        max-width: 100%;
        overflow-x: auto;
        display: block;
    }
    
    table.console-table th,
    table.console-table td {
        border: 1px solid #4a4a4a;
        padding: 0px 8px;
        vertical-align: top;
        white-space: pre-wrap;
        word-break: break-word;
        min-width: 80px;
    }
    
    table.console-table th {
        background-color: #2d2d2d;
        font-weight: bold;
        color: #ffffff;
        position: sticky;
        top: 0;
    }
    
    table.console-table tr:nth-child(even) {
        background-color: #252526;
    }
    
    table.console-table tr:nth-child(odd) {
        background-color: #1e1e1e;
    }

    /* Стили для JSON Viewer */
    .json-viewer {
        margin: 4px 0;
        border-left: 3px solid #569cd6;
				display: block;
    }
    
    .json-viewer .json-node {
        padding: 2px 0;
    }
    
    .json-viewer .json-key {
        color: #9cdcfe;
        font-weight: bold;
    }
    
    .json-viewer .json-string {
        color: #ce9178;
    }
    
    .json-viewer .json-number {
        color: #b5cea8;
    }
    
    .json-viewer .json-boolean {
        color: #569cd6;
        font-weight: bold;
    }
    
    .json-viewer .json-null {
        color: #808080;
        font-style: italic;
    }
    
    .json-viewer .json-bracket {
        color: #d4d4d4;
        font-weight: bold;
    }
    
    .json-viewer .json-punctuation {
        color: #d4d4d4;
    }
    
    .json-viewer .json-more {
        color: #6a9955;
        font-style: italic;
        padding-left: 20px;
    }
    
    .json-viewer .json-children {
        padding-left: 20px;
        border-left: 1px solid #2d2d30;
        margin: 2px 0 2px 5px;
    }
    
    /* Стили для разных статусов */
    .json-viewer-success {
        border-left-color: #66cc66;
    }
    
    .json-viewer-error {
        border-left-color: #f44336;
    }
    
    .json-viewer-warning {
        border-left-color: #FFE66D;
    }
    
    /* Подсветка при наведении для QTextEdit (поддерживается) */
    .json-viewer .json-node:hover {
        background-color: #2d2d30;
        border-radius: 2px;
    }
)";
}

class CommandConsole::Private {
public:
	Private(CommandConsole* parent) :	q(parent) { }
	CommandConsole* q;

	CommandContext* context = nullptr;
	CommandController* controller = nullptr;

	// UI элементы
	QTextEdit* outputArea = nullptr;
	QTextEdit* inputLine = nullptr;
	QVBoxLayout* layout = nullptr;
	QSplitter* splitter = nullptr;
	QVBoxLayout* buttonsLayout = nullptr;
	QWidget* buttonsContainer = nullptr;
	QHBoxLayout* mainLayout = nullptr;
	QWidget* consoleWidget = nullptr;

	// История команд
	QStringList commandHistory;
	int historyIndex = -1; // -1 = после последней команды
	int maxBlocks = 1000;

	void appendTable(const QString& message, const QString& styleClass);
	void appendJson(const QString& message, const QString& styleClass);

	void appendOutputHtml(const QString& html);
	void cleanLatestMessages();

	QToolButton* addButton(const QString& title, const QString& tooltip);
	void setupButtons();
	void setupUi();
	QWidget* setupConsoleUI(QWidget* parent);
	void addToHistory(const QString& command);
	QString getHistoryEntry(int offset);
	void executeCommand(const QString& command);
	void focusInput();
	void submitCommend();

	// Стилизация вывода
	void setupOutputStyling();
};

CommandConsole::CommandConsole(CommandController* controller, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent, Qt::WindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint))
{
	d->controller = controller;
	d->context = controller->commandContext();

	d->setupUi();
	d->setupOutputStyling();

	// Подключение сигналов контекста
	connect(d->context, &CommandContext::outputRequested, this, &CommandConsole::onOutputRequested);

	// Приветственное сообщение
	appendMessage("Command Console ready. Type 'help' for available commands.", "system");
}

CommandConsole::~CommandConsole() = default;

void CommandConsole::Private::setupButtons() {
	connect(addButton("⎚", "Очистить консоль"), &QToolButton::clicked, q, &CommandConsole::onOutputClear);
	//connect(addButton("📥", "Загрузить"), &QToolButton::clicked, q, &CodeEditorWindow::onOpenDocumentClick);
}

QToolButton* CommandConsole::Private::addButton(const QString& title, const QString& tooltip) {
	auto btn = new QToolButton(buttonsContainer);
	buttonsLayout->insertWidget(buttonsLayout->count() - 1, btn);
	btn->setText(title);
	btn->setToolTip(tooltip);
	btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
	btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	return btn;
}

QWidget* CommandConsole::Private::setupConsoleUI(QWidget* parent) {
	using namespace Extensions;

	// Область вывода
	outputArea = new QTextEdit(parent);
	outputArea->setReadOnly(true);
	outputArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	outputArea->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	TextEdit::setTabDistance(outputArea, 2);

	// Поле ввода
	inputLine = new QTextEdit(parent);
	inputLine->setPlaceholderText("Enter command...");
	inputLine->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	inputLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	TextEdit::setTabDistance(inputLine, 2);

	// Создаем сплиттер
	splitter = new QSplitter(Qt::Vertical, parent);
	splitter->addWidget(outputArea);
	splitter->addWidget(inputLine);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 0);
	splitter->setCollapsible(0, false);
	splitter->setCollapsible(1, false);

	// Или можно задать минимальные размеры
	outputArea->setMinimumHeight(100);
	inputLine->setMinimumHeight(50);

	// Макет для размещения сплиттера
	layout = new QVBoxLayout(parent);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);
	layout->addWidget(splitter);

	// Горячие клавиши истории
	inputLine->installEventFilter(parent);

	return splitter;
}

void CommandConsole::Private::setupUi() {
	q->setWindowTitle("Command Console");
	q->resize(600, 250);

	mainLayout = new QHBoxLayout(q);
	mainLayout->setContentsMargins(4, 4, 4, 4);
	mainLayout->setSpacing(4);
	consoleWidget = new QWidget(q);
	consoleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QVBoxLayout* consoleLayout = new QVBoxLayout(consoleWidget);
	consoleLayout->setContentsMargins(0, 0, 0, 0);
	consoleLayout->setSpacing(0);

	auto console = setupConsoleUI(consoleWidget);
	consoleLayout->addWidget(console);

	// === Правая часть: кнопки ===
	buttonsContainer = new QWidget(q);
	buttonsContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	buttonsLayout = new QVBoxLayout(buttonsContainer);
	buttonsLayout->setContentsMargins(0, 0, 0, 0);
	buttonsLayout->setSpacing(4);
	buttonsLayout->addStretch();
	setupButtons();
	buttonsLayout->addStretch();

	mainLayout->addWidget(consoleWidget, 1);
	mainLayout->addWidget(buttonsContainer, 0);

	// Горячие клавиши истории
	inputLine->installEventFilter(q);
}

bool CommandConsole::eventFilter(QObject* obj, QEvent* event) {
	if (obj == d->inputLine && event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->key() == Qt::Key_Return && keyEvent->modifiers() == Qt::ShiftModifier) {
			d->submitCommend();
			return true;
		}
	}
	return QWidget::eventFilter(obj, event);
}

void CommandConsole::Private::setupOutputStyling() {
	outputArea->setStyleSheet(kQTextEditOutputAreaStyle);
	outputArea->document()->setDefaultStyleSheet(kQTextEditOutputAreaDocumentStyleSheet);
}

void CommandConsole::Private::submitCommend() {
	QString command = inputLine->document()->toPlainText();
	if (command.isEmpty()) {
		return;
	}

	addToHistory(command);
	inputLine->clear();
	historyIndex = -1; // сброс позиции в истории

	executeCommand(command);
}

void CommandConsole::Private::executeCommand(const QString& command) {
	// Отображение команды в выводе
	QString timestamp = QDateTime::currentDateTime()
		.toString("HH:mm:ss");

	QString safeCommand = command.toHtmlEscaped();
	QString html = QString("<br><div class=\"command\">"
		"<span class=\"timestamp\">[%1]</span> "
		"&gt; %2"
		"</div>")
		.arg(timestamp)
		.arg(safeCommand);

	QTextCursor cursor = outputArea->textCursor();
	cursor.movePosition(QTextCursor::End);
	outputArea->setTextCursor(cursor);
	outputArea->insertHtml(html);

	// Выполнение
	if (controller && controller->commandProcessor()) {
		controller->commandProcessor()->execute(command, context);
	}
	else {
		q->appendMessage("Error: Command processor not available", "error");
	}
}

void CommandConsole::onHistoryUp() {
	if (d->commandHistory.isEmpty()) {
		return;
	}

	if (d->historyIndex <= 0) {
		d->historyIndex = d->commandHistory.size() - 1;
	}
	else {
		--d->historyIndex;
	}

	d->inputLine->setText(d->getHistoryEntry(0));
}

void CommandConsole::onHistoryDown() {
	if (d->commandHistory.isEmpty()) {
		return;
	}

	if (d->historyIndex >= static_cast<int>(d->commandHistory.size()) - 1) {
		d->historyIndex = -1;
		d->inputLine->clear();
	}
	else {
		++d->historyIndex;
		d->inputLine->setText(d->getHistoryEntry(0));
	}
}

void CommandConsole::Private::cleanLatestMessages() {
	QTextDocument* doc = outputArea->document();
	int excess = doc->blockCount() - maxBlocks;
	if (excess > 0) {
		QTextCursor cursor(doc);
		cursor.beginEditBlock();
		cursor.movePosition(QTextCursor::Start);

		for (int i = 0; i < excess; i++) {
			cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
		}

		cursor.removeSelectedText();
		cursor.endEditBlock();
	}
}

void CommandConsole::onOutputRequested(const QString& message, const QString& styleClass, const QString& type) {
	if (type == kCommandPrintStyle_Plane) appendMessage(message, styleClass);
	else if (type == kCommandPrintStyle_Table) d->appendTable(message, styleClass);
	else if (type == kCommandPrintStyle_Json) d->appendJson(message, styleClass);

	d->cleanLatestMessages();
	auto sb = d->outputArea->verticalScrollBar();
	sb->setValue(sb->maximum());
}

void CommandConsole::Private::addToHistory(const QString& command) {
	// Не добавлять дубликаты подряд
	if (!commandHistory.isEmpty() && commandHistory.last() == command) {
		return;
	}

	commandHistory.append(command);
	// Ограничение размера истории
	if (commandHistory.size() > 100) {
		commandHistory.removeFirst();
	}
}

QString CommandConsole::Private::getHistoryEntry(int offset) {
	int index = historyIndex + offset;
	if (index < 0 || index >= commandHistory.size()) {
		return QString();
	}
	return commandHistory.at(index);
}

void CommandConsole::Private::appendOutputHtml(const QString& html) {
	QTextCursor cursor = outputArea->textCursor();
	cursor.beginEditBlock();
	cursor.movePosition(QTextCursor::End);
	cursor.insertHtml(html);
	cursor.endEditBlock();
	outputArea->setTextCursor(cursor);
}

void CommandConsole::Private::appendTable(const QString& message, const QString& styleClass) {
	QString html = QString("<div class=\"%1\">%2</div>")
		.arg(styleClass)
		.arg(message);

	appendOutputHtml(html);
}

void CommandConsole::Private::appendJson(const QString& message, const QString& styleClass) {
	QString html = QString("<br/><div class=\"%1\">%2</div>")
		.arg(styleClass)
		.arg(message);

	appendOutputHtml(html);
}

void CommandConsole::appendMessage(const QString& message, const QString& styleClass) {
	QString safeMessage = message.toHtmlEscaped();
	safeMessage
		.replace("\r\n", "\n")
		.replace('\n', "<br>");
	safeMessage.replace('\t', "&nbsp;&nbsp;");
	QString html = QString("<div class=\"%1\"><br/>%2</div>")
		.arg(styleClass)
		.arg(safeMessage);

	d->appendOutputHtml(html);
}

void CommandConsole::showConsole() {
	show();
	raise();
	activateWindow();
	d->focusInput();
}

void CommandConsole::hideConsole() {
	hide();
}

bool CommandConsole::isVisible() const {
	return QWidget::isVisible();
}

void CommandConsole::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	d->focusInput();
}

void CommandConsole::hideEvent(QHideEvent* event) {
	QWidget::hideEvent(event);
	// Возврат фокуса в главное окно
	if (parentWidget()) {
		parentWidget()->setFocus();
	}
}

void CommandConsole::Private::focusInput() {
	inputLine->setFocus();
	inputLine->selectAll();
}

void CommandConsole::onOutputClear() {
	d->outputArea->clear();
}
