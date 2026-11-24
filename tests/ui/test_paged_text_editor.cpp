#include "Content/TextEditorModule/documents/paged_text_document.h"
#include "Content/TextEditorModule/widgets/paged_text_editor.h"

#include <QFile>
#include <QFileInfo>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextCodec>
#include <QtTest/QtTest>

class TestPagedTextEditor : public QObject {
	Q_OBJECT

private slots:
	void initTestCase();
	void opensLargeFileInBackground();
	void selectionAndCursor();
	void searchCountsInBackground();
	void searchFindsAfterTabs();
	void exposesSearchProperties();
	void reopensInAnotherEncoding();
	void reportsUnsupportedEncoding();
};

namespace {

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
	QCOMPARE(groups.size(), 1);
	QCOMPARE(groups.first().id, QStringLiteral("search"));
	QCOMPARE(groups.first().title, QStringLiteral("Параметры поиска"));
	QVERIFY(groups.first().editor);
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
