#include "Content/CodeEditorWidget/widgets/paged_text_editor.h"

#include <QFile>
#include <QFileInfo>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestPagedTextEditor : public QObject {
	Q_OBJECT

private slots:
	void initTestCase();
	void opensLargeFileInBackground();
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
