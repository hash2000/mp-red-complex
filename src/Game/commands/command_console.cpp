#include "Game/commands/command_console.h"
#include "Game/commands/command_context.h"
#include "Game/commands/command_processor.h"
#include "Game/app_controller.h"

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

class CommandConsole::Private {
public:
	Private(CommandConsole* parent, ApplicationController* controller)
	:	q(parent)
	, controller(controller)
	, context(new CommandContext(controller, parent)) {	

	}

	CommandConsole* q;

	ApplicationController* controller;
	CommandContext* context;

	// UI элементы
	QTextEdit* outputArea;
	QLineEdit* inputLine;
	QVBoxLayout* layout;

	// История команд
	QStringList commandHistory;
	int historyIndex = -1; // -1 = после последней команды

	// Автодополнение
	QCompleter* completer;
	QStringListModel* completerModel;
};


CommandConsole::CommandConsole(ApplicationController* controller, QWidget* parent)
	: d(new Private(this, controller))
	, QWidget(parent, Qt::WindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint))
{

	setupUi();
	setupCompleter();
	setupOutputStyling();

	// Подключение сигналов контекста
	connect(d->context, &CommandContext::outputRequested,
		this, &CommandConsole::onOutputRequested);

	// Приветственное сообщение
	appendMessage("Command Console ready. Type 'help' for available commands.", "system");
}

CommandConsole::~CommandConsole() = default;

void CommandConsole::setupUi() {
	setWindowTitle("Command Console");
	resize(600, 250);

	// Область вывода
	d->outputArea = new QTextEdit(this);
	d->outputArea->setReadOnly(true);
	d->outputArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	d->outputArea->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	// Поле ввода
	d->inputLine = new QLineEdit(this);
	d->inputLine->setPlaceholderText("Enter command...");
	d->inputLine->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	// Макет
	d->layout = new QVBoxLayout(this);
	d->layout->setContentsMargins(4, 4, 4, 4);
	d->layout->setSpacing(4);
	d->layout->addWidget(d->outputArea, 1);
	d->layout->addWidget(d->inputLine, 0);

	// Подключение сигналов ввода
	connect(d->inputLine, &QLineEdit::returnPressed, this, &CommandConsole::onCommandSubmitted);
	connect(d->inputLine, &QLineEdit::textChanged, this, &CommandConsole::onCommandTextChanged);

	// Горячие клавиши истории
	d->inputLine->installEventFilter(this);
}

bool CommandConsole::eventFilter(QObject* obj, QEvent* event) {
	if (obj == d->inputLine && event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->key() == Qt::Key_Up) {
			onHistoryUp();
			return true;
		}
		else if (keyEvent->key() == Qt::Key_Down) {
			onHistoryDown();
			return true;
		}
		else if (keyEvent->key() == Qt::Key_Tab && !d->inputLine->text().trimmed().isEmpty()) {
			// Принудительное завершение по Tab
			if (d->completer && d->completer->completionCount() > 0) {
				d->completer->complete();
				return true;
			}
		}
	}
	return QWidget::eventFilter(obj, event);
}

void CommandConsole::setupCompleter() {
	d->completerModel = new QStringListModel(this);
	d->completer = new QCompleter(d->completerModel, this);
	d->completer->setCaseSensitivity(Qt::CaseInsensitive);
	d->completer->setCompletionMode(QCompleter::PopupCompletion);
	d->completer->setFilterMode(Qt::MatchStartsWith);

	d->inputLine->setCompleter(d->completer);

	// Обновление списка команд для автодополнения
	QTimer::singleShot(0, this, [this]() {
		if (d->controller && d->controller->commandProcessor()) {
			d->completerModel->setStringList(
				d->controller->commandProcessor()->availableCommands()
			);
		}
		});

	connect(d->completer, QOverload<const QString&>::of(&QCompleter::activated),
		this, &CommandConsole::onCompleterActivated);
}

void CommandConsole::setupOutputStyling() {
	// CSS-стилизация вывода
	QString styleSheet = R"(
        QTextEdit {
            background-color: #1e1e1e;
            color: #d4d4d4;
            border: 1px solid #3e3e42;
            border-radius: 3px;
        }
    )";
	d->outputArea->setStyleSheet(styleSheet);

	// Настройка форматирования текста
	d->outputArea->document()->setDefaultStyleSheet(R"(
        .timestamp { color: #808080; font-size: 8pt; }
        .command { color: #569cd6; font-weight: bold; }
        .error { color: #f44336; }
        .success { color: #66cc66; }
        .system { color: #79b8ff; font-style: italic; }
        .info { color: #d4d4d4; }
    )");
}

void CommandConsole::onCommandSubmitted() {
	QString command = d->inputLine->text().trimmed();
	if (command.isEmpty()) {
		return;
	}

	addToHistory(command);
	d->inputLine->clear();
	d->historyIndex = -1; // сброс позиции в истории

	executeCommand(command);
}

void CommandConsole::executeCommand(const QString& command) {
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

	QTextCursor cursor = d->outputArea->textCursor();
	cursor.movePosition(QTextCursor::End);
	d->outputArea->setTextCursor(cursor);
	d->outputArea->insertHtml(html);

	// Выполнение
	if (d->controller && d->controller->commandProcessor()) {
		d->controller->commandProcessor()->execute(command, d->context);
	}
	else {
		appendMessage("Error: Command processor not available", "error");
	}
}

void CommandConsole::onCommandTextChanged(const QString& text) {
	// Обновление подсказок при необходимости
	Q_UNUSED(text)
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

	d->inputLine->setText(getHistoryEntry(0));
	d->inputLine->selectAll(); // для удобства редактирования
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
		d->inputLine->setText(getHistoryEntry(0));
	}
}

void CommandConsole::onCompleterActivated(const QString& text) {
	Q_UNUSED(text)
		// Фокус остаётся в поле ввода, можно продолжить ввод
}

void CommandConsole::onOutputRequested(const QString& message, const QString& styleClass) {
	appendMessage(message, styleClass);
}

void CommandConsole::addToHistory(const QString& command) {
	// Не добавлять дубликаты подряд
	if (!d->commandHistory.isEmpty() && d->commandHistory.last() == command) {
		return;
	}

	d->commandHistory.append(command);
	// Ограничение размера истории
	if (d->commandHistory.size() > 100) {
		d->commandHistory.removeFirst();
	}
}

QString CommandConsole::getHistoryEntry(int offset) {
	int index = d->historyIndex + offset;
	if (index < 0 || index >= d->commandHistory.size()) {
		return QString();
	}
	return d->commandHistory.at(index);
}

void CommandConsole::appendMessage(const QString& message, const QString& styleClass) {
	QString safeMessage = message.toHtmlEscaped();
	safeMessage
		.replace("\r\n", "\n")
		.replace('\n', "<br>");
	safeMessage.replace('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
	QString html = QString("<br><div class=\"%1\">%2</div>")
		.arg(styleClass)
		.arg(safeMessage);

	QTextCursor cursor = d->outputArea->textCursor();
	cursor.movePosition(QTextCursor::End);
	d->outputArea->setTextCursor(cursor);
	d->outputArea->insertHtml(html);

	auto sb = d->outputArea->verticalScrollBar();
	sb->setValue(sb->maximum());
}

void CommandConsole::showConsole() {
	show();
	raise();
	activateWindow();
	focusInput();
}

void CommandConsole::hideConsole() {
	hide();
}

bool CommandConsole::isVisible() const {
	return QWidget::isVisible();
}

void CommandConsole::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	focusInput();
}

void CommandConsole::hideEvent(QHideEvent* event) {
	QWidget::hideEvent(event);
	// Возврат фокуса в главное окно
	if (parentWidget()) {
		parentWidget()->setFocus();
	}
}

void CommandConsole::focusInput() {
	d->inputLine->setFocus();
	d->inputLine->selectAll();
}
