#pragma once
#include <QAbstractScrollArea>
#include <QByteArray>
#include <QString>
#include <QVector>

#include <memory>

class QWheelEvent;

/// Прототип виртуального редактора для очень больших файлов.
///
/// В памяти держится только индекс начал строк и несколько страниц текста;
/// остальное читается с диска по мере прокрутки. Пока режим только для
/// просмотра: правка, выделение и поиск появятся следующими шагами.
class PagedTextEditor : public QAbstractScrollArea {
	Q_OBJECT
public:
	explicit PagedTextEditor(QWidget* parent = nullptr);
	~PagedTextEditor() override;

	/// Открыть файл. Индексация строк идёт в фоновом потоке.
	/// Поддерживаются UTF-8 и однобайтовые кодировки (QTextCodec);
	/// UTF-16/UTF-32 в прототипе не поддерживаются.
	void openFile(const QString& path, const QString& encoding = QStringLiteral("UTF-8"));
	void closeFile();

	QString filePath() const;
	bool isIndexing() const;
	int lineCount() const;
	/// Длина самой длинной строки (в байтах; для UTF-8/ASCII совпадает
	/// с числом символов). Считается при индексации.
	int maxLineLength() const;

	int firstVisibleLine() const;
	/// Текст строки (читается из страницы; для тестов и копирования).
	QString lineText(int line) const;

	/// Позиция курсора: строка и колонка (в символах, с нуля).
	int cursorLine() const;
	int cursorColumn() const;
	/// Номер экранной строки внутри логической (с учётом WordWrap).
	int cursorVisualRow() const;
	bool isCursorVisible() const;
	void setCursorPosition(int line, int column, bool keepAnchor = false);
	void selectAll();
	/// Выделенный текст (многострочный — через '\n').
	QString selectedText() const;

	/// Кодировка, которой открыт файл.
	QString encoding() const;
	/// Переоткрыть текущий файл в другой кодировке.
	void setEncoding(const QString& encoding);

	int fontSize() const;
	void setFontSize(int size);
	bool wordWrap() const;
	void setWordWrap(bool enabled);
	bool lineNumbersVisible() const;
	void setLineNumbersVisible(bool visible);

	/// Поиск по строкам: счётчик считается в фоне, подсвечиваются
	/// только видимые вхождения.
	void setSearchQuery(const QString& query);
	QString searchQuery() const;
	int searchMatchCount() const;
	int currentSearchIndex() const;
	void searchNext();
	void searchPrevious();
	void setSearchCaseSensitive(bool enabled);
	bool searchCaseSensitive() const;

signals:
	/// Индексация завершилась; false — файл не открыт или кодировка не та.
	void indexingFinished(bool success, const QString& errorMessage);
	void lineCountChanged(int lines);
	void cursorPositionChanged(int line, int column);
	void searchCountChanged(int matches);
	void searchCurrentChanged(int current, int total);
	void fontSizeChanged(int size);
	void wordWrapChanged(bool enabled);
	void lineNumbersVisibleChanged(bool visible);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void scrollContentsBy(int dx, int dy) override;
	void keyPressEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
