#include "Content/TextEditorModule/documents/paged_text_document.h"
#include "Libs/Base/background_task_registry.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/language_info.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_highlighter.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScrollBar>
#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextCodec>
#include <QWheelEvent>
#include <QtTest/QtTest>

class TestPagedTextEditor : public QObject {
	Q_OBJECT

private slots:
	void initTestCase();
	void opensLargeFileInBackground();
	void selectionAndCursor();
	void searchCountsInBackground();
	void searchJumpWithWordWrapKeepsMatchVisible();
	void searchFindsAfterTabs();
	void exposesSearchProperties();
	void fontSizeAndWordWrap();
	void gutterClickSelectsLine();
	void longLineLoadsHorizontally();
	void shiftSelectionCrossesLines();
	void highlightingSpansByRules();
	void searchReportsBackgroundProgress();
	void switchFileUpdatesTitle();
	void reopensInAnotherEncoding();
	void reportsUnsupportedEncoding();
	void insertTextAndUndo();
	void enterAndBackspaceChangeLines();
	void pasteFromClipboard();
	void cutCopiesToClipboard();
	void typingMergesIntoOneUndoStep();
	void modifiedPagesMap();
	void saveWritesModifications();
	void saveKeepsCrlf();
	void saveKeepsBom();
	void saveAsCreatesFile();
	void undoRestoresAcrossPageBoundary();
	void deleteAllPagesAndUndo();
	void readOnlyBlocksEditing();
	void searchReflectsEdits();
	void documentTracksModificationAndSaves();
	void startsEditableWithoutFile();
	void documentStartsEditableWithoutFile();
	void spillsDirtyPagesToTemporaryFile();
	void spilledBufferSaves();
	void spillFileIsRemovedAutomatically();
	void newDocumentSpillsLargeContent();
};

namespace {

class TestHighlightingPlugin : public HighlightingPlugin {
public:
	TestHighlightingPlugin() {
		Rule rule;
		rule.pattern = QRegularExpression(QStringLiteral("\\bfoo\\b"));
		QTextCharFormat format;
		format.setForeground(Qt::red);
		rule.format = format;
		addRule(rule);
	}

	LanguageInfo languageInfo() const override {
		LanguageInfo info;
		info.name = QStringLiteral("Test");
		info.mimeType = QStringLiteral("text/x-test");
		return info;
	}

	void install() override {
	}
};

QString makeLine(int index) {
	return QStringLiteral("line %1: quick brown fox jumps over the lazy dog")
		.arg(index, 6, 10, QLatin1Char('0'));
}

QStringList makeLines(int count) {
	QStringList lines;
	lines.reserve(count);
	for (int i = 0; i < count; i++) {
		lines.append(makeLine(i));
	}
	return lines;
}

QString writeTextFile(const QString& dirPath, const QString& name,
	const QByteArray& data) {
	const QString path = QDir(dirPath).filePath(name);
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return QString();
	}
	file.write(data);
	file.close();
	return path;
}

QByteArray readAllBytes(const QString& path) {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return {};
	}
	return file.readAll();
}

} // namespace

void TestPagedTextEditor::initTestCase() {
}

void TestPagedTextEditor::opensLargeFileInBackground() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("large.txt"));

	const int lineCount = 20000;
	QStringList lines;
	lines.reserve(lineCount);
	for (int i = 0; i < lineCount; i++) {
		lines.append(makeLine(i));
	}

	const QByteArray data = lines.join(QLatin1Char('\n')).toUtf8();
	QCOMPARE(data.count('\n'), lineCount - 1);

	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	QCOMPARE(file.write(data), static_cast<qint64>(data.size()));
	file.close();
	QCOMPARE(QFileInfo(path).size(), static_cast<qint64>(data.size()));

	PagedTextEditor editor;
	editor.resize(900, 600);

	QSignalSpy finished(&editor, &PagedTextEditor::indexingFinished);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 15000);
	QCOMPARE(finished.count(), 1);
	QVERIFY(finished.first().at(0).toBool());

	QCOMPARE(editor.lineCount(), lineCount);
	QCOMPARE(editor.lineText(0), lines.first());
	QCOMPARE(editor.lineText(lineCount - 1), lines.last());
	QCOMPARE(editor.maxLineLength(), lines.first().size());

	// Прокрутка к середине: страница читается с диска по требованию.
	editor.verticalScrollBar()->setValue(10000);
	QCOMPARE(editor.firstVisibleLine(), 10000);
	QCOMPARE(editor.lineText(10000), lines.at(10000));

	// В памяти кэш страниц, а не весь файл: после перехода доступны
	// строки из загруженной страницы.
	QCOMPARE(editor.lineText(10001), lines.at(10001));
}

void TestPagedTextEditor::selectionAndCursor() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("select.txt"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write("alpha beta gamma\ndelta epsilon\n");
	file.close();

	PagedTextEditor editor;
	editor.resize(800, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(0, 6);
	QCOMPARE(editor.cursorLine(), 0);
	QCOMPARE(editor.cursorColumn(), 6);

	// Выделение от «beta» до конца слова.
	editor.setCursorPosition(0, 10, true);
	QCOMPARE(editor.selectedText(), QStringLiteral("beta"));

	editor.setCursorPosition(0, 0);
	editor.setCursorPosition(1, 0, true);
	QCOMPARE(editor.selectedText(), QStringLiteral("alpha beta gamma\n"));
}

void TestPagedTextEditor::searchCountsInBackground() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("search.txt"));

	QStringList lines;
	for (int i = 0; i < 500; i++) {
		lines.append(QStringLiteral("needle %1 needle").arg(i));
	}
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write(lines.join(QLatin1Char('\n')).toUtf8());
	file.close();

	PagedTextEditor editor;
	editor.resize(800, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setSearchQuery(QStringLiteral("needle"));
	QTRY_COMPARE_WITH_TIMEOUT(editor.searchMatchCount(), 1000, 15000);
	QVERIFY(editor.currentSearchIndex() >= 0);

	QSignalSpy currentSpy(&editor, &PagedTextEditor::searchCurrentChanged);
	editor.setCursorPosition(250, 0);
	QVERIFY(currentSpy.count() > 0);
	QCOMPARE(currentSpy.last().at(0).toInt(), 500);
	QCOMPARE(currentSpy.last().at(1).toInt(), 1000);

	editor.searchNext();
	QVERIFY(editor.currentSearchIndex() >= 0);
	QVERIFY(!editor.selectedText().isEmpty());

	editor.setSearchQuery(QString());
	QCOMPARE(editor.searchMatchCount(), 0);
}

void TestPagedTextEditor::searchJumpWithWordWrapKeepsMatchVisible() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("wrap-search.txt"));

	QStringList lines;
	for (int i = 0; i < 40; i++) {
		lines.append(QStringLiteral("line %1 with a very long prefix before needle "
			"and some trailing text after it").arg(i));
	}
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write(lines.join(QLatin1Char('\n')).toUtf8());
	file.close();

	PagedTextEditor editor;
	editor.resize(260, 240);
	editor.setWordWrap(true);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setSearchQuery(QStringLiteral("needle"));
	QTRY_COMPARE_WITH_TIMEOUT(editor.searchMatchCount(), 40, 10000);

	for (int i = 0; i < 12; i++) {
		editor.searchNext();
		QVERIFY2(editor.isCursorVisible(),
			qPrintable(QStringLiteral("match %1 is outside the viewport").arg(i)));
	}
}

void TestPagedTextEditor::searchFindsAfterTabs() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("ninja.log"));

	// Строка в стиле .ninja_log: поля разделены табуляцией.
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write("0\t1\t2\tRedComplexBase\tbuild\n"
		"4\t5\t6\tLibs\tproj\n");
	file.close();

	PagedTextEditor editor;
	editor.resize(800, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setSearchQuery(QStringLiteral("build"));
	QTRY_COMPARE_WITH_TIMEOUT(editor.searchMatchCount(), 1, 10000);
	QVERIFY(editor.currentSearchIndex() >= 0);
	// Выделение и копирование идут по исходным колонкам, таб — один символ.
	QCOMPARE(editor.selectedText(), QStringLiteral("build"));

	editor.setSearchQuery(QStringLiteral("Libs"));
	QTRY_COMPARE_WITH_TIMEOUT(editor.searchMatchCount(), 1, 10000);
	QCOMPARE(editor.selectedText(), QStringLiteral("Libs"));

	// После ручного переноса курсора переход идёт от него, а не от старой
	// позиции поиска.
	editor.setSearchQuery(QStringLiteral("build"));
	QTRY_COMPARE_WITH_TIMEOUT(editor.searchMatchCount(), 1, 10000);
	editor.setCursorPosition(1, 0);
	editor.searchNext();
	QCOMPARE(editor.selectedText(), QStringLiteral("build"));
}

void TestPagedTextEditor::exposesSearchProperties() {
	PagedTextDocument document;
	const auto groups = document.createPropertyGroups(nullptr);
	QCOMPARE(groups.size(), 2);
	QCOMPARE(groups.at(0).id, QStringLiteral("info"));
	QCOMPARE(groups.at(1).id, QStringLiteral("search"));
	QCOMPARE(groups.at(1).title, QStringLiteral("Параметры поиска"));
	QVERIFY(groups.at(1).editor);
}

void TestPagedTextEditor::fontSizeAndWordWrap() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("wrap.txt"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write("short\nvery long line that should be wrapped when word wrap is enabled");
	file.close();

	PagedTextEditor editor;
	editor.resize(320, 300);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	QVERIFY(!editor.wordWrap());
	editor.setWordWrap(true);
	QVERIFY(editor.wordWrap());
	QCOMPARE(editor.lineCount(), 2);

	const int size = editor.fontSize();
	QWheelEvent wheel(QPointF(10, 10), QPointF(10, 10), QPoint(),
		QPoint(0, 120), Qt::NoButton, Qt::ControlModifier,
		Qt::NoScrollPhase, false);
	QApplication::sendEvent(editor.viewport(), &wheel);
	QCOMPARE(editor.fontSize(), size + 1);
	editor.setWordWrap(false);
	QVERIFY(!editor.wordWrap());

	QVERIFY(editor.lineNumbersVisible());
	editor.setLineNumbersVisible(false);
	QVERIFY(!editor.lineNumbersVisible());
	editor.setLineNumbersVisible(true);
	QVERIFY(editor.lineNumbersVisible());
}

void TestPagedTextEditor::gutterClickSelectsLine() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("gutter.txt"));
	const QString longLine = QStringLiteral(
		"first line that is long enough to wrap into several visual rows when narrow");
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write((longLine + "\nsecond line\n").toUtf8());
	file.close();

	PagedTextEditor editor;
	editor.resize(220, 300);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);
	editor.setWordWrap(true);

	QTest::mouseClick(editor.viewport(), Qt::LeftButton, Qt::NoModifier,
		QPoint(3, 4));
	QCOMPARE(editor.selectedText(), longLine);
	QCOMPARE(editor.cursorLine(), 0);

	// Клик по второй экранной строке переноса должен попадать в её колонку,
	// а не в начало логической строки.
	QTest::mouseClick(editor.viewport(), Qt::LeftButton, Qt::NoModifier,
		QPoint(80, 20));
	QCOMPARE(editor.cursorLine(), 0);
	QVERIFY(editor.cursorColumn() > 10);

	// Стрелки вверх/вниз ходят по экранным строкам переноса.
	editor.setCursorPosition(0, 30);
	QCOMPARE(editor.cursorVisualRow(), 1);
	QTest::keyClick(&editor, Qt::Key_Up);
	QCOMPARE(editor.cursorLine(), 0);
	QCOMPARE(editor.cursorVisualRow(), 0);
	QTest::keyClick(&editor, Qt::Key_Down);
	QCOMPARE(editor.cursorLine(), 0);
	QCOMPARE(editor.cursorVisualRow(), 1);
}

void TestPagedTextEditor::longLineLoadsHorizontally() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("longline.txt"));
	const QString longLine(5000, QLatin1Char('x'));

	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write(longLine.toUtf8());
	file.close();

	PagedTextEditor editor;
	editor.resize(600, 300);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	QCOMPARE(editor.lineText(0).size(), 5000);
	QVERIFY(editor.displayedLineText(0).size() < longLine.size());

	QVERIFY(editor.horizontalScrollBar()->maximum() > 0);
	const int before = editor.displayedLineText(0).size();
	editor.horizontalScrollBar()->setValue(editor.horizontalScrollBar()->maximum());
	QTRY_VERIFY(editor.displayedLineText(0).size() > before);
	QCOMPARE(editor.lineText(0).size(), 5000);
}

void TestPagedTextEditor::shiftSelectionCrossesLines() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("shift.txt"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write("one\ntwo\n");
	file.close();

	PagedTextEditor editor;
	editor.resize(400, 300);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(0, 0);
	for (int i = 0; i < 4; i++) {
		QTest::keyClick(&editor, Qt::Key_Right, Qt::ShiftModifier);
	}
	QCOMPARE(editor.cursorLine(), 1);
	QCOMPARE(editor.cursorColumn(), 0);
	QCOMPARE(editor.selectedText(), QStringLiteral("one\n"));

	QTest::keyClick(&editor, Qt::Key_Left, Qt::ShiftModifier);
	QCOMPARE(editor.cursorLine(), 0);
	QCOMPARE(editor.cursorColumn(), 3);
	QCOMPARE(editor.selectedText(), QStringLiteral("one"));
}

void TestPagedTextEditor::highlightingSpansByRules() {
	TestHighlightingPlugin plugin;
	PagedTextHighlighter highlighter;
	highlighter.setPlugin(&plugin);

	const auto spans = highlighter.spansForLine(QStringLiteral("foo bar foo"));
	QCOMPARE(spans.size(), 2);
	QCOMPARE(spans.at(0).start, 0);
	QCOMPARE(spans.at(0).length, 3);
	QCOMPARE(spans.at(1).start, 8);
	QCOMPARE(spans.at(1).length, 3);

	PagedTextEditor editor;
	QVERIFY(editor.setHighlightingPlugin(&plugin));
	QCOMPARE(editor.highlightingPlugin(), &plugin);
	QVERIFY(!editor.setHighlightingPlugin(&plugin));
}

void TestPagedTextEditor::searchReportsBackgroundProgress() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("background-search.txt"));

	QStringList lines;
	for (int i = 0; i < 20000; i++) {
		lines.append(makeLine(i));
	}
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write(lines.join(QLatin1Char('\n')).toUtf8());
	file.close();

	PagedTextEditor editor;
	editor.resize(500, 300);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 15000);

	QSignalSpy progress(&editor, &PagedTextEditor::searchProgressChanged);
	editor.setSearchQuery(QStringLiteral("quick"));
	QVERIFY(BackgroundTaskRegistry::instance().taskCount() > 0);
	QTRY_VERIFY_WITH_TIMEOUT(editor.searchMatchCount() > 0, 15000);
	QTRY_COMPARE_WITH_TIMEOUT(BackgroundTaskRegistry::instance().taskCount(), 0, 15000);
	QVERIFY(progress.count() > 0);
}

void TestPagedTextEditor::switchFileUpdatesTitle() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto first = dir.filePath(QStringLiteral("first.log"));
	const auto second = dir.filePath(QStringLiteral("second.log"));
	for (const auto& path : { first, second }) {
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));
		file.write("line\n");
	}

	PagedTextDocument document;
	document.openFile(first);
	QCOMPARE(document.title(), QStringLiteral("first.log"));

	document.openFile(second);
	QCOMPARE(document.title(), QStringLiteral("second.log"));
}

void TestPagedTextEditor::reopensInAnotherEncoding() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("cp1251.txt"));

	QTextCodec* cp1251 = QTextCodec::codecForName("Windows-1251");
	QVERIFY(cp1251);
	{
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));
		file.write(cp1251->fromUnicode(QStringLiteral("привет\nмир")));
	}

	PagedTextEditor editor;
	editor.resize(800, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);
	QVERIFY(editor.lineText(0) != QStringLiteral("привет"));

	editor.setEncoding(QStringLiteral("Windows-1251"));
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);
	QCOMPARE(editor.encoding(), QStringLiteral("Windows-1251"));
	QCOMPARE(editor.lineText(0), QStringLiteral("привет"));
	QCOMPARE(editor.lineText(1), QStringLiteral("мир"));
}

void TestPagedTextEditor::reportsUnsupportedEncoding() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("small.txt"));

	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write("hello");
	file.close();

	PagedTextEditor editor;
	QSignalSpy finished(&editor, &PagedTextEditor::indexingFinished);
	editor.openFile(path, QStringLiteral("UTF-16"));

	QCOMPARE(finished.count(), 1);
	QCOMPARE(finished.first().at(0).toBool(), false);
	QVERIFY(!finished.first().at(1).toString().isEmpty());
	QCOMPARE(editor.lineCount(), 0);
}

void TestPagedTextEditor::insertTextAndUndo() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("edit.txt"),
		QByteArray("alpha beta\ngamma\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(800, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	QVERIFY(!editor.isModified());
	QCOMPARE(editor.modifiedPageCount(), 0);

	editor.setCursorPosition(0, 5);
	QVERIFY(editor.insertText(QStringLiteral(" X")));
	QCOMPARE(editor.lineText(0), QStringLiteral("alpha X beta"));
	QCOMPARE(editor.cursorColumn(), 7);
	QVERIFY(editor.isModified());
	QCOMPARE(editor.modifiedPageCount(), 1);

	QVERIFY(editor.canUndo());
	QVERIFY(editor.undo());
	QCOMPARE(editor.lineText(0), QStringLiteral("alpha beta"));
	QVERIFY(!editor.isModified());
	QCOMPARE(editor.modifiedPageCount(), 0);
	QCOMPARE(editor.cursorLine(), 0);
	QCOMPARE(editor.cursorColumn(), 5);

	QVERIFY(editor.canRedo());
	QVERIFY(editor.redo());
	QCOMPARE(editor.lineText(0), QStringLiteral("alpha X beta"));
	QVERIFY(editor.isModified());
}

void TestPagedTextEditor::enterAndBackspaceChangeLines() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("lines.txt"),
		QByteArray("one\ntwo\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);
	QCOMPARE(editor.lineCount(), 3);

	editor.setCursorPosition(0, 3);
	QTest::keyClick(&editor, Qt::Key_Return);
	QCOMPARE(editor.lineCount(), 4);
	QCOMPARE(editor.cursorLine(), 1);
	QCOMPARE(editor.lineText(0), QStringLiteral("one"));
	QCOMPARE(editor.lineText(1), QString());

	QTest::keyClicks(&editor, QStringLiteral("hi"));
	QCOMPARE(editor.lineText(1), QStringLiteral("hi"));

	QTest::keyClick(&editor, Qt::Key_Backspace);
	QCOMPARE(editor.lineText(1), QStringLiteral("h"));
	QTest::keyClick(&editor, Qt::Key_Backspace);
	QCOMPARE(editor.lineText(1), QString());
	QTest::keyClick(&editor, Qt::Key_Backspace);
	QCOMPARE(editor.lineCount(), 3);
	QCOMPARE(editor.lineText(0), QStringLiteral("one"));
	QCOMPARE(editor.cursorLine(), 0);
	QCOMPARE(editor.cursorColumn(), 3);

	// Delete в конце строки убирает перевод строки и склеивает строки.
	QTest::keyClick(&editor, Qt::Key_Delete);
	QCOMPARE(editor.lineCount(), 2);
	QCOMPARE(editor.lineText(0), QStringLiteral("onetwo"));
}

void TestPagedTextEditor::pasteFromClipboard() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("paste.txt"),
		QByteArray("start\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	// Буфер обмена может содержать CRLF: в модели всегда '\n'.
	QGuiApplication::clipboard()->setText(QStringLiteral("A\r\nB\nC"));
	editor.setCursorPosition(0, 0);
	QTest::keyClick(&editor, Qt::Key_V, Qt::ControlModifier);

	QCOMPARE(editor.lineCount(), 4);
	QCOMPARE(editor.lineText(0), QStringLiteral("A"));
	QCOMPARE(editor.lineText(1), QStringLiteral("B"));
	QCOMPARE(editor.lineText(2), QStringLiteral("Cstart"));
	QCOMPARE(editor.lineText(3), QString());
	QVERIFY(editor.isModified());

	QVERIFY(editor.undo());
	QCOMPARE(editor.lineCount(), 2);
	QCOMPARE(editor.lineText(0), QStringLiteral("start"));

	// Вставка методом редактора работает так же, как Ctrl+V.
	QVERIFY(editor.insertText(QStringLiteral("xy")));
	QCOMPARE(editor.lineText(0), QStringLiteral("xystart"));
}

void TestPagedTextEditor::cutCopiesToClipboard() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("cut.txt"),
		QByteArray("cut me\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(0, 0);
	editor.setCursorPosition(0, 3, true);
	QVERIFY(editor.hasSelection());
	QTest::keyClick(&editor, Qt::Key_X, Qt::ControlModifier);
	QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("cut"));
	QCOMPARE(editor.lineText(0), QStringLiteral(" me"));
	QVERIFY(editor.isModified());

	// Копирование текст не меняет.
	editor.setCursorPosition(0, 0);
	editor.setCursorPosition(0, 3, true);
	QTest::keyClick(&editor, Qt::Key_C, Qt::ControlModifier);
	QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral(" me"));
	QCOMPARE(editor.lineText(0), QStringLiteral(" me"));
}

void TestPagedTextEditor::typingMergesIntoOneUndoStep() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("typed.txt"),
		QByteArray("abc\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(0, 3);
	QTest::keyClicks(&editor, QStringLiteral("xyz"));
	QCOMPARE(editor.lineText(0), QStringLiteral("abcxyz"));

	QVERIFY(editor.undo());
	QCOMPARE(editor.lineText(0), QStringLiteral("abc"));
	QVERIFY(!editor.canUndo());
	QCOMPARE(editor.cursorColumn(), 3);

	QVERIFY(editor.redo());
	QCOMPARE(editor.lineText(0), QStringLiteral("abcxyz"));
}

void TestPagedTextEditor::modifiedPagesMap() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QStringList lines = makeLines(6000);
	const auto path = writeTextFile(dir.path(), QStringLiteral("pages.txt"),
		lines.join(QLatin1Char('\n')).toUtf8());
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);
	QCOMPARE(editor.lineCount(), 6000);

	// Правка строки 10 держит в памяти только первую страницу.
	editor.setCursorPosition(10, 0);
	QVERIFY(editor.insertText(QStringLiteral(">")));
	QCOMPARE(editor.lineText(10), QStringLiteral(">") + lines.at(10));
	QCOMPARE(editor.modifiedPageCount(), 1);
	// Незатронутая страница по-прежнему читается с диска.
	QCOMPARE(editor.lineText(3500), lines.at(3500));

	editor.setCursorPosition(4100, 0);
	QVERIFY(editor.insertText(QStringLiteral("<")));
	QCOMPARE(editor.modifiedPageCount(), 2);
	QCOMPARE(editor.lineText(4100), QStringLiteral("<") + lines.at(4100));

	QVERIFY(editor.undo());
	QCOMPARE(editor.modifiedPageCount(), 1);
	QVERIFY(editor.undo());
	QCOMPARE(editor.modifiedPageCount(), 0);
	QCOMPARE(editor.lineText(10), lines.at(10));
	QVERIFY(!editor.isModified());
}

void TestPagedTextEditor::saveWritesModifications() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const int count = 6000;
	const QStringList lines = makeLines(count);
	const auto path = writeTextFile(dir.path(), QStringLiteral("save.txt"),
		lines.join(QLatin1Char('\n')).toUtf8());
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(0, 0);
	QVERIFY(editor.insertText(QStringLiteral("HEAD ")));
	editor.setCursorPosition(4100, 0);
	QVERIFY(editor.insertText(QStringLiteral(">")));
	editor.setCursorPosition(5000, 0);
	QVERIFY(editor.insertText(QStringLiteral("NEW\n")));
	QCOMPARE(editor.lineCount(), count + 1);

	QVERIFY(editor.save());
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 15000);
	QVERIFY(!editor.isModified());
	QCOMPARE(editor.modifiedPageCount(), 0);
	QCOMPARE(editor.lineCount(), count + 1);
	QCOMPARE(editor.lineText(0), QStringLiteral("HEAD ") + lines.at(0));
	QCOMPARE(editor.lineText(4100), QStringLiteral(">") + lines.at(4100));
	QCOMPARE(editor.lineText(5000), QStringLiteral("NEW"));
	QCOMPARE(editor.lineText(5001), lines.at(5000));

	const QList<QByteArray> savedLines = readAllBytes(path).split('\n');
	QCOMPARE(int(savedLines.size()), count + 1);
	QCOMPARE(QString::fromUtf8(savedLines.at(0)), QStringLiteral("HEAD ") + lines.at(0));
	QCOMPARE(QString::fromUtf8(savedLines.at(5000)), QStringLiteral("NEW"));
	QCOMPARE(QString::fromUtf8(savedLines.at(5001)), lines.at(5000));
}

void TestPagedTextEditor::saveKeepsCrlf() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("crlf.txt"),
		QByteArray("first\r\nsecond\r\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);
	QCOMPARE(editor.lineText(0), QStringLiteral("first"));

	editor.setCursorPosition(0, 0);
	QVERIFY(editor.insertText(QStringLiteral(">")));
	QVERIFY(editor.save());
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	// Неизменённая часть и переводы строк остались прежними.
	QCOMPARE(readAllBytes(path), QByteArray(">first\r\nsecond\r\n"));
}

void TestPagedTextEditor::saveKeepsBom() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("bom.txt"),
		QByteArray("\xEF\xBB\xBF") + QByteArray("bom\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);
	// BOM не попадает в текст строки.
	QCOMPARE(editor.lineText(0), QStringLiteral("bom"));

	editor.setCursorPosition(0, 0);
	QVERIFY(editor.insertText(QStringLiteral(">")));
	QVERIFY(editor.save());
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	const QByteArray saved = readAllBytes(path);
	QVERIFY(saved.startsWith(QByteArray("\xEF\xBB\xBF")));
	QCOMPARE(saved.mid(3), QByteArray(">bom\n"));
}

void TestPagedTextEditor::saveAsCreatesFile() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.newDocument();
	QCOMPARE(editor.lineCount(), 1);
	QVERIFY(!editor.isModified());

	QVERIFY(editor.insertText(QStringLiteral("hello\nworld")));
	QCOMPARE(editor.lineCount(), 2);
	QVERIFY(editor.isModified());

	const auto path = dir.filePath(QStringLiteral("created.txt"));
	QVERIFY(editor.saveAs(path));
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	QVERIFY(!editor.isModified());
	QCOMPARE(editor.filePath(), path);
#ifdef Q_OS_WIN
	// У нового файла нет исходного перевода строки: берётся платформенный.
	QCOMPARE(readAllBytes(path), QByteArray("hello\r\nworld"));
#else
	QCOMPARE(readAllBytes(path), QByteArray("hello\nworld"));
#endif
	QCOMPARE(editor.lineText(0), QStringLiteral("hello"));
	QCOMPARE(editor.lineText(1), QStringLiteral("world"));
}

void TestPagedTextEditor::undoRestoresAcrossPageBoundary() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const int count = 5000;
	const QStringList lines = makeLines(count);
	const auto path = writeTextFile(dir.path(), QStringLiteral("boundary.txt"),
		lines.join(QLatin1Char('\n')).toUtf8());
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(1995, 0);
	editor.setCursorPosition(2005, 0, true);
	QVERIFY(editor.insertText(QStringLiteral("X")));

	QCOMPARE(editor.lineCount(), count - 10);
	QCOMPARE(editor.lineText(1995), QStringLiteral("X") + lines.at(2005));
	QCOMPARE(editor.lineText(1996), lines.at(2006));

	QVERIFY(editor.undo());
	QCOMPARE(editor.lineCount(), count);
	QCOMPARE(editor.lineText(1995), lines.at(1995));
	QCOMPARE(editor.lineText(2004), lines.at(2004));
	QCOMPARE(editor.lineText(2005), lines.at(2005));
	QCOMPARE(editor.cursorLine(), 2005);
	QCOMPARE(editor.cursorColumn(), 0);
	QVERIFY(!editor.isModified());

	QVERIFY(editor.redo());
	QCOMPARE(editor.lineCount(), count - 10);
	QCOMPARE(editor.lineText(1995), QStringLiteral("X") + lines.at(2005));
}

void TestPagedTextEditor::deleteAllPagesAndUndo() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const int count = 6000;
	const QStringList lines = makeLines(count);
	const auto path = writeTextFile(dir.path(), QStringLiteral("deleteall.txt"),
		lines.join(QLatin1Char('\n')).toUtf8());
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.selectAll();
	QVERIFY(editor.deleteSelection());
	QCOMPARE(editor.lineCount(), 1);
	QCOMPARE(editor.lineText(0), QString());
	QVERIFY(editor.isModified());
	QCOMPARE(editor.modifiedPageCount(), 3);

	// Отмена большого удаления не хранит текст: страницы снова читаются
	// из файла.
	QVERIFY(editor.undo());
	QCOMPARE(editor.lineCount(), count);
	QCOMPARE(editor.lineText(0), lines.at(0));
	QCOMPARE(editor.lineText(count - 1), lines.at(count - 1));
	QVERIFY(!editor.isModified());
	QCOMPARE(editor.modifiedPageCount(), 0);

	QVERIFY(editor.redo());
	QCOMPARE(editor.lineCount(), 1);
}

void TestPagedTextEditor::readOnlyBlocksEditing() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("readonly.txt"),
		QByteArray("readonly\n"));
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setReadOnly(true);
	QVERIFY(!editor.insertText(QStringLiteral("x")));
	QTest::keyClicks(&editor, QStringLiteral("abc"));
	QTest::keyClick(&editor, Qt::Key_Backspace);
	QCOMPARE(editor.lineText(0), QStringLiteral("readonly"));
	QVERIFY(!editor.isModified());

	editor.setReadOnly(false);
	QVERIFY(editor.insertText(QStringLiteral("x")));
	QCOMPARE(editor.lineText(0), QStringLiteral("xreadonly"));
}

void TestPagedTextEditor::searchReflectsEdits() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	QStringList lines;
	for (int i = 0; i < 40; i++) {
		lines.append(QStringLiteral("needle %1").arg(i));
	}
	const auto path = writeTextFile(dir.path(), QStringLiteral("searchedit.txt"),
		lines.join(QLatin1Char('\n')).toUtf8());
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setSearchQuery(QStringLiteral("needle"));
	QTRY_COMPARE_WITH_TIMEOUT(editor.searchMatchCount(), 40, 15000);

	// Заменяем первое совпадение: поиск пересчитывается в фоне и не
	// переводит курсор на найденное.
	editor.setCursorPosition(0, 0);
	editor.setCursorPosition(0, 6, true);
	QVERIFY(editor.insertText(QStringLiteral("hay")));
	QCOMPARE(editor.cursorColumn(), 3);
	QTRY_COMPARE_WITH_TIMEOUT(editor.searchMatchCount(), 39, 15000);
	QCOMPARE(editor.cursorLine(), 0);
	QCOMPARE(editor.cursorColumn(), 3);
}

void TestPagedTextEditor::documentTracksModificationAndSaves() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = writeTextFile(dir.path(), QStringLiteral("document.txt"),
		QByteArray("doc\n"));
	QVERIFY(!path.isEmpty());

	PagedTextDocument document;
	document.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!document.editor()->isIndexing(), 10000);
	QVERIFY(!document.isModified());
	QCOMPARE(document.title(), QFileInfo(path).fileName());

	document.editor()->setCursorPosition(0, 0);
	QVERIFY(document.editor()->insertText(QStringLiteral("new ")));
	QVERIFY(document.isModified());

	QVERIFY(document.save());
	QTRY_VERIFY_WITH_TIMEOUT(!document.editor()->isIndexing(), 15000);
	QVERIFY(!document.isModified());
	QCOMPARE(document.title(), QFileInfo(path).fileName());
	QCOMPARE(document.editor()->filePath(), path);
	QCOMPARE(readAllBytes(path), QByteArray("new doc\n"));
}

void TestPagedTextEditor::startsEditableWithoutFile() {
	PagedTextEditor editor;
	editor.resize(600, 400);

	// Документ без файла — это одна пустая строка, а не пустой просмотрщик.
	QCOMPARE(editor.lineCount(), 1);
	QVERIFY(!editor.isIndexing());
	QVERIFY(!editor.isModified());
	QCOMPARE(editor.filePath(), QString());

	QVERIFY(editor.insertText(QStringLiteral("hello\nworld")));
	QCOMPARE(editor.lineCount(), 2);
	QCOMPARE(editor.lineText(0), QStringLiteral("hello"));
	QCOMPARE(editor.lineText(1), QStringLiteral("world"));
	QVERIFY(editor.isModified());

	// Enter, Backspace и буфер обмена работают так же, как с файлом.
	editor.setCursorPosition(1, 5);
	QTest::keyClick(&editor, Qt::Key_Return);
	QCOMPARE(editor.lineCount(), 3);
	QTest::keyClick(&editor, Qt::Key_Backspace);
	QCOMPARE(editor.lineCount(), 2);

	QGuiApplication::clipboard()->setText(QStringLiteral("!"));
	editor.setCursorPosition(0, 0);
	QTest::keyClick(&editor, Qt::Key_V, Qt::ControlModifier);
	QCOMPARE(editor.lineText(0), QStringLiteral("!hello"));

	QVERIFY(editor.undo());
	QCOMPARE(editor.lineText(0), QStringLiteral("hello"));
}

void TestPagedTextEditor::documentStartsEditableWithoutFile() {
	PagedTextDocument document;
	QVERIFY(document.editor() != nullptr);
	QCOMPARE(document.editor()->lineCount(), 1);
	QCOMPARE(document.title(), QStringLiteral("Без имени"));

	document.editor()->setCursorPosition(0, 0);
	QVERIFY(document.editor()->insertText(QStringLiteral("документ")));
	QCOMPARE(document.editor()->lineText(0), QStringLiteral("документ"));
	QVERIFY(document.isModified());

	QVERIFY(document.editor()->undo());
	QVERIFY(!document.isModified());
}

void TestPagedTextEditor::spillsDirtyPagesToTemporaryFile() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const int count = 5000;
	const QStringList lines = makeLines(count);
	const auto path = writeTextFile(dir.path(), QStringLiteral("spill.txt"),
		lines.join(QLatin1Char('\n')).toUtf8());
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.setMemoryBudget(32 * 1024);
	QCOMPARE(editor.memoryBudget(), qint64(32 * 1024));

	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(10, 0);
	QVERIFY(editor.insertText(QStringLiteral("A")));
	QCOMPARE(editor.lineText(10), QStringLiteral("A") + lines.at(10));
	QVERIFY2(editor.spilledPageCount() > 0,
		"изменённая страница должна уйти во временный файл");

	const QString spillPath = editor.spillFilePath();
	QVERIFY(!spillPath.isEmpty());
	QVERIFY(QFile::exists(spillPath));
	// В памяти — страницы, а не весь файл.
	QVERIFY(editor.bufferedBytes() < 2 * 1024 * 1024);

	// Вторая правка страницы, которая уже лежит в файле подкачки.
	QVERIFY(editor.insertText(QStringLiteral("BC")));
	QCOMPARE(editor.lineText(10), QStringLiteral("ABC") + lines.at(10));
	QCOMPARE(editor.modifiedPageCount(), 1);

	// Снимки для отмены читаются из файла подкачки.
	QVERIFY(editor.undo());
	QCOMPARE(editor.lineText(10), QStringLiteral("A") + lines.at(10));
	QVERIFY(editor.undo());
	QCOMPARE(editor.lineText(10), lines.at(10));
	QVERIFY(!editor.isModified());
	QCOMPARE(editor.spilledPageCount(), 0);

	// Незатронутая страница по-прежнему читается из файла.
	QCOMPARE(editor.lineText(4500), lines.at(4500));
}

void TestPagedTextEditor::spilledBufferSaves() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const int count = 5000;
	const QStringList lines = makeLines(count);
	const auto path = writeTextFile(dir.path(), QStringLiteral("spillsave.txt"),
		lines.join(QLatin1Char('\n')).toUtf8());
	QVERIFY(!path.isEmpty());

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.setMemoryBudget(16 * 1024);
	editor.openFile(path);
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 10000);

	editor.setCursorPosition(10, 0);
	QVERIFY(editor.insertText(QStringLiteral("A")));
	editor.setCursorPosition(3100, 0);
	QVERIFY(editor.insertText(QStringLiteral("B")));
	QVERIFY(editor.spilledPageCount() >= 2);

	// Сохранение читает выгруженные страницы из файла подкачки.
	QVERIFY(editor.save());
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 15000);
	QVERIFY(!editor.isModified());
	QCOMPARE(editor.lineText(10), QStringLiteral("A") + lines.at(10));
	QCOMPARE(editor.lineText(3100), QStringLiteral("B") + lines.at(3100));
	QVERIFY(editor.spillFilePath().isEmpty());

	const QList<QByteArray> savedLines = readAllBytes(path).split('\n');
	QCOMPARE(QString::fromUtf8(savedLines.at(10)), QStringLiteral("A") + lines.at(10));
	QCOMPARE(QString::fromUtf8(savedLines.at(3100)), QStringLiteral("B") + lines.at(3100));
}

void TestPagedTextEditor::spillFileIsRemovedAutomatically() {
	QString spillPath;
	{
		PagedTextEditor editor;
		editor.resize(600, 400);
		editor.setMemoryBudget(16 * 1024);
		editor.newDocument();
		QVERIFY(editor.insertText(QString(64 * 1024, QLatin1Char('x'))));
		spillPath = editor.spillFilePath();
		QVERIFY(!spillPath.isEmpty());
		QVERIFY(QFile::exists(spillPath));
	}

	// Файл подкачки удаляется вместе с редактором (QTemporaryFile).
	QVERIFY(!QFile::exists(spillPath));
}

void TestPagedTextEditor::newDocumentSpillsLargeContent() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QStringList lines = makeLines(3000);

	PagedTextEditor editor;
	editor.resize(600, 400);
	editor.setMemoryBudget(64 * 1024);
	editor.newDocument();

	// Без файла всё содержимое — изменённые страницы: большая вставка
	// должна уйти в файл подкачки, а не остаться в памяти.
	QVERIFY(editor.insertText(lines.join(QLatin1Char('\n'))));
	QCOMPARE(editor.lineCount(), 3000);
	QVERIFY(editor.spilledPageCount() > 0);
	QVERIFY(!editor.spillFilePath().isEmpty());
	QVERIFY(editor.bufferedBytes() < 2 * 1024 * 1024);

	const auto path = dir.filePath(QStringLiteral("from-buffer.txt"));
	QVERIFY(editor.saveAs(path));
	QTRY_VERIFY_WITH_TIMEOUT(!editor.isIndexing(), 15000);
	QVERIFY(!editor.isModified());
	QCOMPARE(editor.lineCount(), 3000);
	QCOMPARE(editor.lineText(0), lines.at(0));
	QCOMPARE(editor.lineText(2999), lines.at(2999));
}

QTEST_MAIN(TestPagedTextEditor)
#include "test_paged_text_editor.moc"
