#include "Libs/Engine/ui/command_console.h"
#include "Libs/Engine/ui/name_select_dialog.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Base/extensions/text_edit_extensions.h"
#include "Libs/Resources/resources.h"

#include <QFile>
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
#include <QMessageBox>

namespace {
/// Оформление текста вывода (HTML-классы) хранится в теме:
/// assets/themes/{theme}/stylesheets/console-document.css.
QString loadOutputDocumentStyle(CommandController* controller) {
	if (!controller || !controller->resources()) {
		return { };
	}

	const auto themeDir = controller->resources()->Directories.get("path.theme");
	if (!themeDir) {
		return { };
	}

	QFile file(themeDir->filePath("stylesheets/console-document.css"));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "CommandConsole: console-document.css is not found in theme:" << themeDir->path();
		return { };
	}

	return QString::fromUtf8(file.readAll());
}
}

class CommandConsole::Private {
public:
	Private(CommandConsole* parent) :	q(parent) { }
	CommandConsole* q;

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
	QToolButton* selectNamespaceButton = nullptr;

	// История команд
	QStringList commandHistory;
	int historyIndex = -1; // -1 = после последней команды
	int maxBlocks = 1000;

	QString currentNamespace = ".";

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

	d->setupUi();
	d->setupOutputStyling();

	// Подключение сигналов контекста
	connect(d->controller, &CommandController::outputRequested, this, &CommandConsole::onOutputRequested);

	// Приветственное сообщение
	appendMessage("Command Console ready. Type 'help' for available commands.", "system");
}

CommandConsole::~CommandConsole() = default;

void CommandConsole::Private::setupButtons() {
	connect(addButton("⎚", tr("Очистить консоль")), &QToolButton::clicked, q, &CommandConsole::onOutputClear);
	connect((selectNamespaceButton = addButton("⛏", tr("Выбрать контекст"))), &QToolButton::clicked, q, &CommandConsole::onSelectContext);
	//connect(addButton("📥", "Загрузить"), &QToolButton::clicked, q, &CodeEditorWindow::onOpenDocumentClick);
}

QToolButton* CommandConsole::Private::addButton(const QString& title, const QString& tooltip) {
	auto btn = new QToolButton(buttonsContainer);
	btn->setObjectName(QStringLiteral("ConsoleButton"));
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
	outputArea->setObjectName(QStringLiteral("ConsoleOutputArea"));
	outputArea->setReadOnly(true);
	outputArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	outputArea->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	TextEdit::setTabDistance(outputArea, 2);

	// Поле ввода
	inputLine = new QTextEdit(parent);
	inputLine->setObjectName(QStringLiteral("ConsoleInputLine"));
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

	// Макет для размещения сплиттера: используем уже созданный у parent,
	// иначе виджет останется без разметки.
	layout = qobject_cast<QVBoxLayout*>(parent->layout());
	if (!layout) {
		layout = new QVBoxLayout(parent);
		layout->setContentsMargins(4, 4, 4, 4);
		layout->setSpacing(4);
	}
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
	// Виджеты оформляет тема (console.qss), а HTML-вывод консоли —
	// console-document.css из каталога активной темы.
	outputArea->document()->setDefaultStyleSheet(loadOutputDocumentStyle(controller));
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
		auto ns = controller->globalContext()->context(currentNamespace);
		if (!ns) {
			controller->printError(QString("Unknown context name [%1]")
				.arg(currentNamespace));
			return;
		}

		controller->commandProcessor()->execute(command, ns);
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
	if (type == kCommandPrintStyle_Plane) {
		appendMessage(message, styleClass);
	}
	else if (type == kCommandPrintStyle_Table) {
		d->appendTable(message, styleClass);
	}
	else if (type == kCommandPrintStyle_Json) {
		d->appendJson(message, styleClass);
	}

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
	const auto questionResult = QMessageBox::question(this, "Подтверждение", "Вы уверены, что хотите удалить лог консоли?",
		QMessageBox::Yes | QMessageBox::No);
	if (questionResult == QMessageBox::No) {
		return;
	}
	d->outputArea->clear();
}

void CommandConsole::onSelectContext() {
	const auto names = d->controller->globalContext()->nameSpaces();
	NameSelectDialog dialog(names, this);
	// Позиционируем диалог прямо под кнопкой
	QPoint buttonBottomLeft = d->selectNamespaceButton->mapToGlobal(QPoint(0, d->selectNamespaceButton->height()));
	dialog.move(buttonBottomLeft);

	// Опционально: задаем ширину диалога равной ширине кнопки (если список длинный, лучше оставить фиксированную или побольше)
	// dialog.setFixedWidth(m_selectButton->width()); 

	if (dialog.exec() == QDialog::Accepted) {
		QString selectedVar = dialog.selectedName();

		// Обновляем UI
		d->selectNamespaceButton->setToolTip(selectedVar);
		d->currentNamespace = selectedVar;
		d->controller->printSystem(QString("use context [%1]")
			.arg(selectedVar));
	}
}
