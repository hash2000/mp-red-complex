#pragma once
#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <memory>

class PagedSpillStore;

/// Снимок одной страницы для истории правок.
///
/// Чистая страница не хранит текст: он читается из файла, поэтому в снимке
/// достаточно признака `dirty == false`. Именно это делает отмену больших
/// правок дешёвой: страницы, которые до правки были чистыми, при отмене
/// снова становятся «читай из файла».
struct PagedPageSnapshot {
	QStringList lines;
	QString newline;
	bool loaded = false;
	bool dirty = false;
};

/// Виртуальная модель текста виртуального редактора.
///
/// Файл не читается целиком: `PagedFileIndexer` отдаёт смещения начал строк,
/// страницы по 2000 строк загружаются по требованию, а изменённые страницы
/// остаются в памяти отдельными буферами — это и есть карта «страница →
/// изменённый буфер». Неизменённые страницы по-прежнему читаются с диска,
/// поэтому память растёт вместе с объёмом правок, а не с размером файла.
///
/// Объём изменённых страниц ограничен `memoryBudget()`. Когда предел
/// превышен, страницы выгружаются в временный файл (`QTemporaryFile` в
/// системном каталоге временных файлов) и при следующем обращении читаются
/// оттуда; файл удаляется автоматически, когда модель (или её копия для
/// фонового поиска) уничтожается. Такой файл нужен и документу без файла:
/// его содержимое целиком живёт в изменённых страницах.
class PagedDocumentBuffer {
public:
	/// Сколько исходных строк файла покрывает одна страница.
	static constexpr int kPageLines = 2000;
	/// Предел памяти под изменённые страницы по умолчанию (32 МиБ).
	static constexpr qint64 kDefaultMemoryBudget = 32 * 1024 * 1024;

	void reset(const QString& filePath, const QByteArray& encodingName,
		const QVector<qint64>& starts, qint64 size, int baseMaxLineLength = 0);
	/// Пустой документ без файла: одна пустая строка.
	void resetEmpty(const QByteArray& encodingName);
	void clear();

	bool isValid() const;
	QString path() const;
	QByteArray encodingName() const;
	bool hasFile() const;

	/// Текущее число строк (после правок оно может отличаться от исходного).
	int lineCount() const;
	/// Верхняя оценка самой длинной строки: для исходного файла точная,
	/// после правок не уменьшается (полный пересчёт потребовал бы чтения
	/// всех страниц).
	int maxLineLength() const;
	/// Текст строки; страница подгружается с диска или из файла подкачки.
	QString lineAt(int line) const;

	/// Заменить диапазон текстом `text` (координаты — логическая строка и
	/// колонка в символах). Пустой `text` означает удаление. Возвращает false,
	/// если координаты некорректны или замена ничего не меняет.
	bool replace(int startLine, int startColumn, int endLine, int endColumn,
		const QString& text);

	int pageCount() const;
	/// Страницы, которых касается диапазон логических строк.
	QVector<int> pagesForLines(int startLine, int endLine) const;
	/// Состояние страницы до/после правки (см. PagedPageSnapshot).
	PagedPageSnapshot pageSnapshot(int page) const;
	/// Восстановить состояние страницы (отмена и повтор правки).
	void restorePage(int page, const PagedPageSnapshot& snapshot);

	/// Сколько страниц изменено в памяти (ещё не записано в файл).
	int dirtyPageCount() const;
	bool hasChanges() const;

	/// Предел памяти под изменённые страницы; 0 — без предела.
	void setMemoryBudget(qint64 bytes);
	qint64 memoryBudget() const;
	/// Сколько памяти занимают изменённые страницы сейчас.
	qint64 bufferedBytes() const;
	/// Сколько изменённых страниц имеет запись в файле подкачки.
	int spilledPageCount() const;
	/// Путь временного файла подкачки; пустая строка — файл ещё не создан.
	QString spillFilePath() const;
	/// Выгрузить изменённые страницы, пока память не уложится в предел.
	void enforceMemoryBudget() const;

	/// Записать текущее состояние в файл. Неизменённые страницы копируются
	/// с диска байт в байт, изменённые — кодируются и разделяются тем же
	/// переводом строки, что был в файле. Возвращает false и заполняет
	/// `error` при ошибке записи.
	/// @param lineTransform если задан, каждая строка пропускается через него
	/// перед кодированием (например, замена ведущих табов пробелами);
	/// в этом случае файл переписывается построчно целиком.
	bool saveTo(const QString& targetPath, QString* error,
		const std::function<QString(const QString&)>& lineTransform = {}) const;

private:
	struct Page {
		/// Индекс первой исходной строки страницы.
		int originalFirst = 0;
		/// Сколько исходных строк покрывает страница.
		int originalCount = 0;
		/// Текущее число строк страницы (после правок может отличаться).
		int currentCount = 0;
		/// Текущее содержимое; действительно только при `loaded`.
		QStringList lines;
		/// Перевод строки, которым страница разделена в файле.
		QString newline;
		bool loaded = false;
		bool dirty = false;
		/// Смещение и размер записи страницы в файле подкачки (-1 — нет).
		qint64 spillOffset = -1;
		qint64 spillBytes = 0;
		/// Память под строки страницы (учитывается только для изменённых).
		qint64 memoryBytes = 0;
		/// Счётчик обращений для выбора страницы при выгрузке.
		quint64 access = 0;
	};

	int countOf(const Page& page) const;
	void invalidateCounts() const;
	void ensurePrefix() const;
	bool locate(int line, int& page, int& index) const;
	void ensureLoaded(int page) const;
	void loadPage(int page) const;
	void trimLoadedPages() const;
	void markDirty(int page);
	void refreshPageMemory(int page) const;
	void updateMaxLineLength(const QStringList& lines);
	void detectFileFacts(const QString& filePath);
	bool spillPage(int page) const;
	void enforceMemoryBudget(int protectedPage) const;
	/// Полная перезапись с преобразованием каждой строки.
	bool saveWithTransform(const QString& targetPath, QString* error,
		const std::function<QString(const QString&)>& lineTransform) const;

	mutable QVector<Page> pages;
	mutable QVector<int> prefix;
	mutable bool prefixDirty = true;
	mutable QList<int> loadedOrder;
	mutable std::shared_ptr<PagedSpillStore> spillStore;
	mutable qint64 dirtyMemory = 0;
	mutable quint64 accessCounter = 0;

	QString filePath;
	QByteArray codecName = "UTF-8";
	QVector<qint64> lineStarts;
	qint64 fileSize = 0;
	QString defaultNewline = QStringLiteral("\n");
	bool bom = false;
	int maxLength = 0;
	int dirtyPages = 0;
	int cachePages = 4;
	qint64 budget = kDefaultMemoryBudget;
};
