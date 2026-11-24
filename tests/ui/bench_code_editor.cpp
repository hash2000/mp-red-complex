// Стенд для измерений производительности редактора кода.
// Не входит в набор штатных тестов: собирается отдельной целью.
#include "Content/TextEditorModule/formatters/formatter_plugin_manager.h"
#include "Content/TextEditorModule/highlights/highlighter.h"
#include "Content/TextEditorModule/highlights/highlighter_plugin_manager.h"
#include "Content/TextEditorModule/widgets/code_editor_widget.h"
#include "Libs/Resources/resources.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextLayout>
#include <QTimer>

#include <cstdio>

namespace {

QString makeText(int bytes) {	QString text;
	text.reserve(bytes);
	const QString paragraph = QStringLiteral(
		"## Раздел\n"
		"Обычный текст с **жирным**, *курсивом* и ссылкой https://example.com.\n"
		"Обычный текст с **жирным**, *курсивом* и ссылкой https://example.com.\n"
		"Обычный текст с **жирным**, *курсивом* и ссылкой https://example.com.\n"
		"Обычный текст с **жирным**, *курсивом* и ссылкой https://example.com.\n"
		"Обычный текст с **жирным**, *курсивом* и ссылкой https://example.com.\n"
		"```cpp\n"
		"int main() { return 0; }\n"
		"```\n"
		"\n");
	while (text.size() < bytes) {
		text += paragraph;
	}
	return text;
}

int formatsOfFirstBlock(QTextDocument* document) {
	const QTextBlock block = document->firstBlock();
	if (!block.isValid() || !block.layout()) {
		return -1;
	}
	return static_cast<int>(block.layout()->formats().size());
}

int formattedBlocks(QTextDocument* document) {
	int count = 0;
	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		if (block.layout() && !block.layout()->formats().isEmpty()) {
			count++;
		}
	}
	return count;
}

/// Документ из нескольких очень длинных строк: разметка без переводов
/// строк, из-за которой редактор уходит в горизонтальную прокрутку.
QString makeLongLines(int bytes, int lines) {
	const QString unit = QStringLiteral(
		"<div class=\"row\"><span>Текст</span><a href=\"https://example.com\">ссылка</a></div>");
	const int perLine = bytes / qMax(1, lines);
	QString text;
	text.reserve(bytes);
	for (int line = 0; line < lines; line++) {
		QString current;
		while (current.size() < perLine) {
			current += unit;
		}
		text += current;
		text += QLatin1Char('\n');
	}
	return text;
}

/// Даёт таймерам (отложенная подсветка видимой области) отработать.
void wait(int ms) {
	QEventLoop loop;
	QTimer::singleShot(ms, &loop, &QEventLoop::quit);
	loop.exec();
}

template <typename Editor>
void measureRaw(const QString& text, const char* name, HighlightingPluginManager& manager) {
	std::printf("%s:\n", name);
	QElapsedTimer timer;

	Editor editor;
	editor.setLineWrapMode(Editor::NoWrap);
	editor.resize(900, 600);

	Highlighter highlighter(&manager, editor.document());
	highlighter.setLanguage(QStringLiteral("md"));

	timer.start();
	editor.setPlainText(text);
	const qint64 setText = timer.elapsed();

	editor.show();
	editor.move(-5000, -5000);
	QCoreApplication::processEvents();
	QCoreApplication::processEvents();

	timer.restart();
	for (int i = 0; i < 40; i++) {
		editor.resize(700 + (i % 2) * 400, 500 + (i % 3) * 100);
		QCoreApplication::processEvents();
	}
	const qint64 resize = timer.elapsed();

	timer.restart();
	for (int i = 0; i < 100; i++) {
		editor.verticalScrollBar()->setValue(editor.verticalScrollBar()->maximum() * i / 100);
		QCoreApplication::processEvents();
	}
	const qint64 scroll = timer.elapsed();

	std::printf("  setPlainText(подсветка всего документа)=%lld ms blocks=%d formats=%d\n",
		static_cast<long long>(setText), editor.document()->blockCount(),
		formatsOfFirstBlock(editor.document()));
	std::printf("  40 resizes=%lld ms  100 scrolls=%lld ms\n",
		static_cast<long long>(resize), static_cast<long long>(scroll));
}

void measureNew(const QString& text, HighlightingPluginManager& highlighting,
	FormatterPluginManager& formatters) {
	std::printf("CodeEditorWidget (новый путь):\n");
	QElapsedTimer timer;

	CodeEditorWidget editor(&highlighting, &formatters);
	editor.setLineWrapMode(QPlainTextEdit::NoWrap);
	editor.resize(900, 600);
	editor.show();
	editor.move(-5000, -5000);
	QCoreApplication::processEvents();
	QCoreApplication::processEvents();

	editor.setLanguage(QStringLiteral("md"));

	timer.start();
	editor.setText(text);
	const qint64 setText = timer.elapsed();
	wait(120);

	// Открытие файла: чтение в отдельном потоке + пачки строк.
	const QString path = QDir::temp().filePath(QStringLiteral("redcomplex-bench-big.md"));
	{
		QFile file(path);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			file.write(text.toUtf8());
		}
	}

	QEventLoop openLoop;
	QObject::connect(&editor, &CodeEditorWidget::fileOpened, &openLoop, &QEventLoop::quit);
	timer.start();
	editor.openFile(path);
	openLoop.exec();
	const qint64 openFile = timer.elapsed();
	wait(120);
	const int formatted = formattedBlocks(editor.document());
	QFile::remove(path);

	// Применение результата форматирования (текст отличается переводом
	// строки, чтобы прошёл реальный путь замены, как после prettier).
	QString formattedText = text;
	formattedText += QLatin1Char('\n');
	timer.restart();
	editor.setText(formattedText);
	const qint64 applyFormat = timer.elapsed();
	wait(120);

	timer.restart();
	for (int i = 0; i < 40; i++) {
		editor.resize(700 + (i % 2) * 400, 500 + (i % 3) * 100);
		QCoreApplication::processEvents();
	}
	const qint64 resize = timer.elapsed();

	timer.restart();
	for (int i = 0; i < 100; i++) {
		editor.verticalScrollBar()->setValue(editor.verticalScrollBar()->maximum() * i / 100);
		QCoreApplication::processEvents();
	}
	const qint64 scroll = timer.elapsed();
	wait(120);

	std::printf("  openFile=%lld ms  setText=%lld ms blocks=%d lazy=%d подсвечено строк=%d\n",
		static_cast<long long>(openFile), static_cast<long long>(setText),
		editor.document()->blockCount(), editor.isHighlightingLazy() ? 1 : 0, formatted);
	std::printf("  применение результата форматирования=%lld ms\n",
		static_cast<long long>(applyFormat));
	std::printf("  40 resizes=%lld ms  100 scrolls=%lld ms\n",
		static_cast<long long>(resize), static_cast<long long>(scroll));
}

void measurePlainLongLines(const QString& text) {
	std::printf("Чистый QPlainTextEdit без подсветки, длинные строки:\n");
	QElapsedTimer timer;

	QPlainTextEdit editor;
	editor.setLineWrapMode(QPlainTextEdit::NoWrap);
	editor.resize(900, 600);
	editor.setPlainText(text);
	editor.show();
	editor.move(-5000, -5000);
	QCoreApplication::processEvents();
	QCoreApplication::processEvents();

	timer.restart();
	for (int i = 0; i < 40; i++) {
		editor.resize(700 + (i % 2) * 400, 500 + (i % 3) * 100);
		QCoreApplication::processEvents();
	}
	const qint64 resize = timer.elapsed();

	auto* horizontal = editor.horizontalScrollBar();
	timer.restart();
	for (int i = 0; i < 100; i++) {
		horizontal->setValue(horizontal->maximum() * i / 100);
		QCoreApplication::processEvents();
	}
	const qint64 hscroll = timer.elapsed();

	std::printf("  40 resizes=%lld ms  100 гор. прокруток=%lld ms\n",
		static_cast<long long>(resize), static_cast<long long>(hscroll));
}

void measureWrappedLongLines(const QString& text) {
	std::printf("QPlainTextEdit с переносом длинных строк:\n");
	QElapsedTimer timer;

	QPlainTextEdit editor;
	editor.setLineWrapMode(QPlainTextEdit::WidgetWidth);
	editor.resize(900, 600);
	editor.setPlainText(text);
	editor.show();
	editor.move(-5000, -5000);
	QCoreApplication::processEvents();
	QCoreApplication::processEvents();

	timer.restart();
	for (int i = 0; i < 40; i++) {
		editor.resize(700 + (i % 2) * 400, 500 + (i % 3) * 100);
		QCoreApplication::processEvents();
	}
	const qint64 resize = timer.elapsed();

	auto* vertical = editor.verticalScrollBar();
	timer.restart();
	for (int i = 0; i < 100; i++) {
		vertical->setValue(vertical->maximum() * i / 100);
		QCoreApplication::processEvents();
	}
	const qint64 vscroll = timer.elapsed();

	std::printf("  40 resizes=%lld ms  100 верт. прокруток=%lld ms\n",
		static_cast<long long>(resize), static_cast<long long>(vscroll));
}

void measureTextEditLongLines(const QString& text) {
	std::printf("Чистый QTextEdit без подсветки, длинные строки:\n");
	QElapsedTimer timer;

	QTextEdit editor;
	editor.setLineWrapMode(QTextEdit::NoWrap);
	editor.resize(900, 600);

	timer.start();
	editor.setPlainText(text);
	const qint64 setText = timer.elapsed();

	editor.show();
	editor.move(-5000, -5000);
	QCoreApplication::processEvents();
	QCoreApplication::processEvents();

	timer.restart();
	for (int i = 0; i < 40; i++) {
		editor.resize(700 + (i % 2) * 400, 500 + (i % 3) * 100);
		QCoreApplication::processEvents();
	}
	const qint64 resize = timer.elapsed();

	auto* horizontal = editor.horizontalScrollBar();
	timer.restart();
	for (int i = 0; i < 100; i++) {
		horizontal->setValue(horizontal->maximum() * i / 100);
		QCoreApplication::processEvents();
	}
	const qint64 hscroll = timer.elapsed();

	std::printf("  setText=%lld ms  40 resizes=%lld ms  100 гор. прокруток=%lld ms\n",
		static_cast<long long>(setText), static_cast<long long>(resize),
		static_cast<long long>(hscroll));
}

void measureLongLines(const QString& text, HighlightingPluginManager& highlighting,
	FormatterPluginManager& formatters) {
	std::printf("CodeEditorWidget, длинные строки (новый путь):\n");
	QElapsedTimer timer;

	CodeEditorWidget editor(&highlighting, &formatters);
	editor.resize(900, 600);
	editor.show();
	editor.move(-5000, -5000);
	QCoreApplication::processEvents();
	QCoreApplication::processEvents();
	editor.setLanguage(QStringLiteral("md"));

	timer.start();
	editor.setText(text);
	const qint64 setText = timer.elapsed();
	wait(120);

	int maxLineLength = 0;
	for (QTextBlock block = editor.document()->begin(); block.isValid(); block = block.next()) {
		maxLineLength = qMax(maxLineLength, block.length());
	}

	timer.restart();
	for (int i = 0; i < 40; i++) {
		editor.resize(700 + (i % 2) * 400, 500 + (i % 3) * 100);
		QCoreApplication::processEvents();
	}
	const qint64 resize = timer.elapsed();

	auto* horizontal = editor.horizontalScrollBar();
	timer.restart();
	for (int i = 0; i < 100; i++) {
		horizontal->setValue(horizontal->maximum() * i / 100);
		QCoreApplication::processEvents();
	}
	const qint64 hscroll = timer.elapsed();

	timer.restart();
	for (int i = 0; i < 100; i++) {
		editor.verticalScrollBar()->setValue(editor.verticalScrollBar()->maximum() * i / 100);
		QCoreApplication::processEvents();
	}
	const qint64 vscroll = timer.elapsed();

	std::printf("  setText=%lld ms blocks=%d maxLine=%d подсвечено строк=%d wrap=%d\n",
		static_cast<long long>(setText), editor.document()->blockCount(), maxLineLength,
		formattedBlocks(editor.document()),
		editor.lineWrapMode() == QPlainTextEdit::WidgetWidth ? 1 : 0);
	std::printf("  40 resizes=%lld ms  100 гор. прокруток=%lld ms  100 верт.=%lld ms\n",
		static_cast<long long>(resize), static_cast<long long>(hscroll),
		static_cast<long long>(vscroll));
}

} // namespace

int main(int argc, char** argv) {
	QApplication app(argc, argv);

	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"),
		QDir(QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR) + QStringLiteral("/assets/themes/dark")));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	for (const int bytes : { 100 * 1024, 440 * 1024 }) {
		const QString text = makeText(bytes);
		std::printf("=== text bytes=%lld utf16=%lld lines=%lld ===\n",
			static_cast<long long>(text.toUtf8().size()),
			static_cast<long long>(text.size()),
			static_cast<long long>(text.count(QLatin1Char('\n'))));

		measureRaw<QTextEdit>(text, "QTextEdit + Highlighter (было)", highlighting);
		measureNew(text, highlighting, formatters);
	}

	// Случай минифицированной разметки: несколько очень длинных строк,
	// прокрутка преимущественно горизонтальная.
	const QString longLines = makeLongLines(440 * 1024, 20);
	std::printf("=== длинные строки: bytes=%lld blocks=%d ===\n",
		static_cast<long long>(longLines.toUtf8().size()),
		static_cast<int>(longLines.count(QLatin1Char('\n'))));
	measureLongLines(longLines, highlighting, formatters);

	// Масштабирование по длине строки — долгий режим (десятки секунд).
	if (app.arguments().contains(QStringLiteral("--scaling"))) {
		for (const int lineChars : { 500, 2000, 5000 }) {
			const int lines = qMax(1, (440 * 1024) / lineChars);
			const QString scalingText = makeLongLines(440 * 1024, lines);
			std::printf("=== строки по ~%d символов: blocks=%d ===\n", lineChars, lines);
			measurePlainLongLines(scalingText);
			measureWrappedLongLines(scalingText);
			measureTextEditLongLines(scalingText);
		}
	}

	return 0;
}
