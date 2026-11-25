#pragma once
#include <QAbstractScrollArea>
#include <QByteArray>
#include <QList>
#include <QPoint>
#include <QString>
#include <QVector>

#include <memory>

class QContextMenuEvent;
class QInputMethodEvent;
class QWheelEvent;
class FormatterPlugin;
class FormatterPluginManager;
class HighlightingPlugin;
class HighlightingPluginManager;

/// Виртуальный редактор для очень больших файлов.
///
/// В памяти держится только индекс начал строк и несколько страниц текста;
/// остальное читается с диска по мере прокрутки. Функциональные блоки:
/// PagedFileIndexer/PagedDocumentBuffer — файл, страницы и карта изменённых
/// страниц, PagedLineChunks — длинные строки, TextLayout — экранные строки и
/// позиция курсора, PagedTextRenderer — отрисовка, PagedSearch — фоновый
/// поиск, PagedEditHistory — отмена и повтор правок.
///
/// Редактирование устроено так же, как просмотр: правка загружает только
/// затронутые страницы (по 2000 строк) и оставляет их в памяти отдельными
/// буферами — карта «страница → изменённый буфер». Остальные страницы
/// по-прежнему читаются из файла, поэтому память растёт вместе с объёмом
/// правок, а не с размером файла. Объём изменённых страниц ограничен
/// `setMemoryBudget()`; при превышении они выгружаются во временный файл в
/// системном каталоге temp и читаются оттуда по требованию. Редактор можно
/// использовать и без файла (`newDocument()`): тогда всё содержимое живёт
/// в изменённых страницах и при необходимости тоже уходит в файл подкачки.
/// При сохранении изменённые страницы кодируются и записываются, а
/// неизменённые копируются байт в байт, так что переводы строк и BOM
/// исходного файла сохраняются. После записи файл переиндексируется в фоне.
///
/// Многокурсорность: Alt+клик добавляет или убирает курсор (на строке — не
/// больше одного), Shift+Alt мышью или стрелками выделяет прямоугольную
/// область от верхнего курсора, Ctrl+D дублирует строку ниже. Копирование и
/// вырезание без выделения работают со строкой целиком, а вставка
/// распределяет строки буфера по курсорам.
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
	/// Пустой документ без файла (кнопка «Новый файл»).
	void newDocument();

	QString filePath() const;
	bool isIndexing() const;
	int lineCount() const;
	/// Длина самой длинной строки (в байтах; для UTF-8/ASCII совпадает
	/// с числом символов). Считается при индексации; после правок — верхняя
	/// оценка, которая не уменьшается.
	int maxLineLength() const;

	int firstVisibleLine() const;
	/// Текст строки (читается из страницы; для тестов и копирования).
	QString lineText(int line) const;

	/// Позиция курсора: строка и колонка (в символах, с нуля).
	int cursorLine() const;
	int cursorColumn() const;
	/// Есть ли выделение (сравниваются позиции, текст не собирается).
	bool hasSelection() const;
	/// Номер экранной строки внутри логической (с учётом WordWrap).
	int cursorVisualRow() const;
	bool isCursorVisible() const;
	/// Отображаемая часть строки (с учётом кусочной загрузки).
	QString displayedLineText(int line) const;
	void setCursorPosition(int line, int column, bool keepAnchor = false);
	void selectAll();
	/// Выделенный текст (многострочный — через '\n').
	QString selectedText() const;

	/// Число курсоров (в обычном режиме — 1).
	int cursorCount() const;
	/// Позиции всех курсоров в текстовом порядке: QPoint(строка, колонка).
	QList<QPoint> cursorPositions() const;
	/// Добавить курсор в позицию. На одной строке — только один курсор;
	/// если на строке уже есть курсор, вызов ничего не меняет.
	void addCursorAt(int line, int column);
	/// Убрать дополнительные курсоры, оставив основной.
	void clearExtraCursors();

	/// Кодировка, которой открыт файл.
	QString encoding() const;
	/// Переоткрыть текущий файл в другой кодировке. При несохранённых
	/// изменениях отказывает (false), чтобы не потерять правки.
	bool setEncoding(const QString& encoding);

	/// Режим «только чтение»: правки запрещены, навигация и копирование
	/// работают как обычно.
	bool isReadOnly() const;
	void setReadOnly(bool readOnly);

	/// Есть ли несохранённые изменения (изменённые страницы в памяти).
	bool isModified() const;
	/// Сколько страниц изменено в памяти — карта «страница → буфер».
	int modifiedPageCount() const;

	/// Предел памяти под изменённые страницы (байт); 0 — без предела.
	/// Когда предел превышен, изменённые страницы выгружаются во временный
	/// файл в системном каталоге temp и читаются оттуда при необходимости.
	void setMemoryBudget(qint64 bytes);
	qint64 memoryBudget() const;
	/// Сколько памяти занимают изменённые страницы сейчас.
	qint64 bufferedBytes() const;
	/// Сколько изменённых страниц имеет запись во временном файле.
	int spilledPageCount() const;
	/// Путь временного файла подкачки; пустая строка — файл ещё не создан.
	QString spillFilePath() const;

	/// Записать изменения в текущий файл. false — файла нет или ошибка
	/// записи (подробности в сигнале saved()).
	bool save();
	/// Записать изменения в указанный файл; работа продолжается с ним.
	bool saveAs(const QString& path);

	bool canUndo() const;
	bool canRedo() const;
	bool undo();
	bool redo();

	/// Вставить текст в позицию курсора (заменяет выделение).
	bool insertText(const QString& text);
	/// Удалить выделение, а при его отсутствии — символ справа.
	bool deleteSelection();
	void copy();
	void cut();
	void paste();

	void setHighlightingManager(HighlightingPluginManager* manager);
	void setFormatterManager(FormatterPluginManager* manager);
	/// Включить подсветку по плагину; nullptr — без подсветки.
	bool setHighlightingPlugin(HighlightingPlugin* plugin);
	HighlightingPlugin* highlightingPlugin() const;
	bool isHighlightingEnabled() const;
	void setHighlightingEnabled(bool enabled);

	/// Доступен ли форматировщик для выбранного языка.
	bool canFormat() const;
	bool isFormatting() const;
	/// Форматирование всего файла с записью результата обратно в файл.
	bool formatDocument();
	qint64 fileSize() const;

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
	/// Промежуточное количество найденных совпадений во время фонового поиска.
	void searchProgressChanged(int matches);
	void searchCurrentChanged(int current, int total);
	void fontSizeChanged(int size);
	void wordWrapChanged(bool enabled);
	void lineNumbersVisibleChanged(bool visible);
	void highlightingChanged();
	void highlightingEnabledChanged(bool enabled);
	void formattingStarted();
	void formattingFinished(bool changed);
	void formattingFailed(const QString& errorMessage);
	/// Изменилось наличие несохранённых правок.
	void modifiedChanged(bool modified);
	void undoAvailableChanged(bool available);
	void redoAvailableChanged(bool available);
	/// Изменённые страницы выгружены во временный файл (pages — сколько
	/// страниц сейчас лежит в файле подкачки; 0 — файла нет).
	void bufferSpilled(int pages);
	/// Файл записан; success=false — текст ошибки в errorMessage.
	void saved(const QString& path, bool success, const QString& errorMessage);

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
	void inputMethodEvent(QInputMethodEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
