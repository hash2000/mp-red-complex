#include "Content/TextEditorModule/documents/paged_text_document.h"
#include "Libs/Base/background_task_registry.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/language_info.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_highlighter.h"

#include <QFile>
#include <QFileInfo>
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

QTEST_MAIN(TestPagedTextEditor)
#include "test_paged_text_editor.moc"
