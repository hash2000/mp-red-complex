#include "Libs/DataStream/paged_text/paged_document_buffer.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTextCodec>
#include <QtGlobal>

#include <algorithm>

namespace {

/// Сколько байт читается при определении BOM и перевода строки.
constexpr qint64 kFactsProbeSize = 8192;

/// Размер порции при копировании неизменённой страницы в новый файл.
constexpr qint64 kCopyChunk = 1 << 20;

/// Версия формата записи страницы в файле подкачки.
constexpr qint32 kSpillRecordVersion = 1;

QString platformNewline() {
#ifdef Q_OS_WIN
	return QStringLiteral("\r\n");
#else
	return QStringLiteral("\n");
#endif
}

QTextCodec* codecForName(const QByteArray& name) {
	QTextCodec* codec = QTextCodec::codecForName(name);
	return codec ? codec : QTextCodec::codecForName("UTF-8");
}

/// Оценка памяти под строки страницы (заголовки QString + символы UTF-16).
qint64 memoryOf(const QStringList& lines) {
	qint64 bytes = lines.size() * static_cast<qint64>(sizeof(QString));
	for (const QString& line : lines) {
		bytes += line.size() * 2 + 24;
	}
	return bytes;
}

} // namespace

/// Файл подкачки изменённых страниц.
///
/// Создаёт временный файл в системном каталоге (`QDir::tempPath()`) и
/// удаляет его автоматически при уничтожении: `QTemporaryFile` с
/// `setAutoRemove(true)` — стандартный механизм Qt, не зависящий от ОС.
/// Если приложение аварийно завершится, файл останется в системном каталоге
/// временных файлов и будет убран штатной очисткой ОС.
///
/// Запись идёт в открытый `QTemporaryFile`, чтение — отдельным `QFile` по
/// пути: так копия модели для фонового поиска может читать страницы, пока
/// редактор дописывает новые записи (файл только растёт).
class PagedSpillStore {
public:
	PagedSpillStore() {
		const QString pattern = QDir(QDir::tempPath()).filePath(
			QStringLiteral("redcomplex-paged-XXXXXX"));
		file.setFileTemplate(pattern);
		file.setAutoRemove(true);
		valid = file.open();
	}

	~PagedSpillStore() = default;

	bool isValid() const {
		return valid;
	}

	QString path() const {
		return valid ? file.fileName() : QString();
	}

	bool write(const QStringList& lines, qint64& offset, qint64& size) {
		if (!valid || !file.isOpen() || !file.seek(file.size())) {
			return false;
		}

		const qint64 start = file.pos();
		{
			QDataStream out(&file);
			out.setVersion(QDataStream::Qt_6_0);
			out << kSpillRecordVersion;
			out << static_cast<qint32>(lines.size());
			for (const QString& line : lines) {
				out << line;
			}
			if (out.status() != QDataStream::Ok) {
				return false;
			}
		}
		// Данные должны быть видны читателю, который откроет файл отдельно.
		file.flush();

		offset = start;
		size = file.pos() - start;
		return size > 0;
	}

	bool read(qint64 offset, qint64 size, QStringList& lines) const {
		lines.clear();
		if (!valid || size <= 0) {
			return valid;
		}

		QFile reader(path());
		if (!reader.open(QIODevice::ReadOnly)) {
			return false;
		}
		if (!reader.seek(offset)) {
			return false;
		}

		QDataStream in(&reader);
		in.setVersion(QDataStream::Qt_6_0);
		qint32 version = 0;
		qint32 count = 0;
		in >> version >> count;
		// Каждая строка в записи занимает минимум 4 байта длины: счётчик,
		// который не может уложиться в размер записи, — признак порчи.
		if (in.status() != QDataStream::Ok
			|| version != kSpillRecordVersion || count < 0
			|| static_cast<qint64>(count) * 4 > size + 16) {
			return false;
		}

		QStringList result;
		result.reserve(count);
		for (qint32 i = 0; i < count; i++) {
			QString line;
			in >> line;
			if (in.status() != QDataStream::Ok) {
				return false;
			}
			result.append(line);
		}

		if (in.status() != QDataStream::Ok) {
			return false;
		}
		lines = result;
		return true;
	}

private:
	QTemporaryFile file;
	bool valid = false;
};

void PagedDocumentBuffer::reset(const QString& path, const QByteArray& encodingName,
	const QVector<qint64>& starts, qint64 size, int baseMaxLineLength) {
	filePath = path;
	codecName = encodingName.isEmpty() ? QByteArray("UTF-8") : encodingName;
	lineStarts = starts;
	if (lineStarts.isEmpty()) {
		// Индексер всегда отдаёт хотя бы одну строку, но пустой файл и
		// вырожденный индекс не должны ронять модель.
		lineStarts.append(0);
	}
	fileSize = qMax<qint64>(0, size);
	maxLength = qMax(0, baseMaxLineLength);
	dirtyPages = 0;
	dirtyMemory = 0;
	accessCounter = 0;
	spillStore.reset();
	pages.clear();
	prefix.clear();
	prefixDirty = true;
	loadedOrder.clear();
	bom = false;
	defaultNewline = platformNewline();
	detectFileFacts(path);

	const int originalLines = lineStarts.size();
	const int pageCount = qMax(1, (originalLines + kPageLines - 1) / kPageLines);
	pages.resize(pageCount);
	for (int i = 0; i < pageCount; i++) {
		Page& page = pages[i];
		page.originalFirst = i * kPageLines;
		page.originalCount = qMin(kPageLines, originalLines - page.originalFirst);
		page.currentCount = page.originalCount;
		page.newline = defaultNewline;
	}
}

void PagedDocumentBuffer::resetEmpty(const QByteArray& encodingName) {
	clear();
	codecName = encodingName.isEmpty() ? QByteArray("UTF-8") : encodingName;
	lineStarts.append(0);
	defaultNewline = platformNewline();
	Page page;
	page.originalFirst = 0;
	page.originalCount = 1;
	page.currentCount = 1;
	page.lines.append(QString());
	page.newline = defaultNewline;
	page.loaded = true;
	pages.append(page);
	prefixDirty = true;
}

void PagedDocumentBuffer::clear() {
	filePath.clear();
	codecName = "UTF-8";
	lineStarts.clear();
	fileSize = 0;
	maxLength = 0;
	dirtyPages = 0;
	dirtyMemory = 0;
	accessCounter = 0;
	spillStore.reset();
	pages.clear();
	prefix.clear();
	prefixDirty = true;
	loadedOrder.clear();
	bom = false;
	defaultNewline = platformNewline();
}

bool PagedDocumentBuffer::isValid() const {
	return !pages.isEmpty();
}

QString PagedDocumentBuffer::path() const {
	return filePath;
}

QByteArray PagedDocumentBuffer::encodingName() const {
	return codecName;
}

bool PagedDocumentBuffer::hasFile() const {
	return !filePath.isEmpty();
}

int PagedDocumentBuffer::countOf(const Page& page) const {
	// Число строк хранится отдельно: изменённая страница может быть
	// выгружена в файл подкачки и не иметь загруженного содержимого.
	return page.currentCount;
}

void PagedDocumentBuffer::invalidateCounts() const {
	prefixDirty = true;
}

void PagedDocumentBuffer::ensurePrefix() const {
	if (!prefixDirty) {
		return;
	}

	prefix.clear();
	prefix.reserve(pages.size() + 1);
	prefix.append(0);
	for (const Page& page : pages) {
		prefix.append(prefix.last() + countOf(page));
	}
	prefixDirty = false;
}

int PagedDocumentBuffer::lineCount() const {
	if (pages.isEmpty()) {
		return 0;
	}
	ensurePrefix();
	return prefix.last();
}

int PagedDocumentBuffer::maxLineLength() const {
	return maxLength;
}

bool PagedDocumentBuffer::locate(int line, int& page, int& index) const {
	if (pages.isEmpty() || line < 0) {
		return false;
	}
	ensurePrefix();
	if (line >= prefix.last()) {
		return false;
	}

	// Ищем последнюю страницу, начало которой не позже строки: страницы
	// нулевой длины (полностью удалённые) так пропускаются автоматически.
	int low = 0;
	int high = pages.size() - 1;
	while (low < high) {
		const int middle = (low + high + 1) / 2;
		if (prefix.at(middle) <= line) {
			low = middle;
		}
		else {
			high = middle - 1;
		}
	}

	page = low;
	index = line - prefix.at(low);
	return index >= 0;
}

void PagedDocumentBuffer::detectFileFacts(const QString& path) {
	if (path.isEmpty()) {
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}

	const QByteArray bytes = file.read(kFactsProbeSize);
	if (bytes.startsWith("\xEF\xBB\xBF")) {
		bom = true;
	}

	const int newline = bytes.indexOf('\n');
	if (newline < 0) {
		return;
	}
	defaultNewline = newline > 0 && bytes.at(newline - 1) == '\r'
		? QStringLiteral("\r\n") : QStringLiteral("\n");
}

void PagedDocumentBuffer::ensureLoaded(int page) const {
	if (page < 0 || page >= pages.size()) {
		return;
	}

	if (!pages.at(page).loaded) {
		loadPage(page);
	}
	pages[page].access = ++accessCounter;
}

void PagedDocumentBuffer::loadPage(int pageIndex) const {
	Page& page = pages[pageIndex];
	QStringList lines;
	QString newline;

	if (page.dirty && page.spillOffset >= 0 && spillStore) {
		// Страница лежит в файле подкачки: возвращаем её в память.
		// При ошибке чтения lines останется пустым и ниже заполнится
		// пустыми строками — модель не должна терять число строк.
		static_cast<void>(spillStore->read(page.spillOffset, page.spillBytes, lines));
		newline = page.newline;
	}
	else if (!filePath.isEmpty() && page.originalCount > 0) {
		QFile file(filePath);
		if (file.open(QIODevice::ReadOnly)) {
			const qint64 begin = lineStarts.value(page.originalFirst, 0);
			const int afterLast = page.originalFirst + page.originalCount;
			const qint64 end = afterLast < lineStarts.size()
				? lineStarts.at(afterLast) : fileSize;
			if (file.seek(begin)) {
				const QByteArray bytes = file.read(qMax<qint64>(0, end - begin));
				QTextCodec* codec = codecForName(codecName);
				QString text = codec ? codec->toUnicode(bytes) : QString::fromUtf8(bytes);
				if (page.originalFirst == 0 && text.startsWith(QChar(0xFEFF))) {
					text.remove(0, 1);
				}

				lines = text.split(QLatin1Char('\n'));
				for (QString& line : lines) {
					if (line.endsWith(QLatin1Char('\r'))) {
						line.chop(1);
					}
				}

				if (bytes.contains("\r\n")) {
					newline = QStringLiteral("\r\n");
				}
				else if (bytes.contains('\n')) {
					newline = QStringLiteral("\n");
				}
			}
		}
	}

	// Файл мог исчезнуть или укоротиться: страница всё равно обязана
	// содержать ровно столько строк, сколько заявлено в модели.
	const int expected = page.currentCount;
	while (lines.size() < expected) {
		lines.append(QString());
	}
	if (lines.size() > expected) {
		lines = lines.mid(0, expected);
	}

	page.lines = lines;
	if (!newline.isEmpty()) {
		page.newline = newline;
	}
	page.loaded = true;
	page.access = ++accessCounter;
	if (page.dirty) {
		refreshPageMemory(pageIndex);
	}
	else {
		page.currentCount = page.lines.size();
	}

	loadedOrder.removeAll(pageIndex);
	loadedOrder.append(pageIndex);
	trimLoadedPages();
}

void PagedDocumentBuffer::trimLoadedPages() const {
	// Вытесняются только чистые страницы: изменённый буфер — это часть
	// модели, но он может быть выгружен в файл подкачки.
	int guard = 0;
	while (loadedOrder.size() > cachePages && guard++ < loadedOrder.size() * 2) {
		const int pageIndex = loadedOrder.takeFirst();
		Page& page = pages[pageIndex];
		if (!page.loaded || page.dirty) {
			continue;
		}
		page.lines.clear();
		page.loaded = false;
	}
}

QString PagedDocumentBuffer::lineAt(int line) const {
	int page = 0;
	int index = 0;
	if (!locate(line, page, index)) {
		return QString();
	}
	ensureLoaded(page);
	// Значение забираем до выгрузки: строка живёт в возвращаемой копии.
	const QString result = pages.at(page).lines.value(index);
	// Одна очень длинная страница может сама не влезть в предел: в этом
	// случае полный перебор страниц на каждую прочитанную строку не нужен.
	const qint64 pageMemory = pages.at(page).memoryBytes;
	if (dirtyMemory - pageMemory > budget) {
		enforceMemoryBudget(page);
	}
	return result;
}

bool PagedDocumentBuffer::replace(int startLine, int startColumn, int endLine,
	int endColumn, const QString& text) {
	if (pages.isEmpty()) {
		return false;
	}

	const int total = lineCount();
	if (total <= 0) {
		return false;
	}

	// Курсор и якорь выделения могут прийти в любом порядке.
	if (startLine > endLine || (startLine == endLine && startColumn > endColumn)) {
		std::swap(startLine, endLine);
		std::swap(startColumn, endColumn);
	}

	startLine = qBound(0, startLine, total - 1);
	endLine = qBound(startLine, endLine, total - 1);

	int startPage = 0;
	int startIndex = 0;
	int endPage = 0;
	int endIndex = 0;
	if (!locate(startLine, startPage, startIndex)
		|| !locate(endLine, endPage, endIndex)) {
		return false;
	}

	ensureLoaded(startPage);
	if (endPage != startPage) {
		ensureLoaded(endPage);
	}

	Page& first = pages[startPage];
	if (startIndex < 0 || startIndex >= first.lines.size()) {
		return false;
	}
	startColumn = qBound(0, startColumn, first.lines.at(startIndex).size());

	Page& last = pages[endPage];
	if (endIndex < 0 || endIndex >= last.lines.size()) {
		return false;
	}
	endColumn = qBound(0, endColumn, last.lines.at(endIndex).size());

	if (startPage == endPage && startIndex == endIndex
		&& startColumn == endColumn && text.isEmpty()) {
		return false;
	}

	QStringList parts = text.isEmpty()
		? QStringList{ QString() } : text.split(QLatin1Char('\n'));

	if (startPage == endPage) {
		QStringList& lines = first.lines;
		const QString head = lines.at(startIndex).left(startColumn);
		const QString tail = lines.at(endIndex).mid(endColumn);
		parts.first().prepend(head);
		parts.last().append(tail);

		const int removeCount = endIndex - startIndex + 1;
		for (int i = 0; i < removeCount; i++) {
			lines.removeAt(startIndex);
		}
		for (int i = 0; i < parts.size(); i++) {
			lines.insert(startIndex + i, parts.at(i));
		}

		markDirty(startPage);
		updateMaxLineLength(parts);
		refreshPageMemory(startPage);
		invalidateCounts();
		enforceMemoryBudget(-1);
		return true;
	}

	// Правка пересекает границу страниц. Строки первой страницы после
	// начала диапазона и последней до конца диапазона исчезают; страницы
	// между ними становятся пустыми буферами.
	QStringList& startLines = first.lines;
	const QString head = startLines.at(startIndex).left(startColumn);
	const QString tail = last.lines.at(endIndex).mid(endColumn);

	// Страницы между первой и последней попадают в удаляемый диапазон
	// целиком: их содержимое выбрасывается, а сами страницы становятся
	// пустыми изменёнными буферами. Текст для этого читать не нужно.
	for (int page = startPage + 1; page < endPage; page++) {
		pages[page].lines.clear();
		pages[page].loaded = true;
		markDirty(page);
		refreshPageMemory(page);
	}

	QStringList newStartLines = startLines.mid(0, startIndex);
	if (parts.size() == 1) {
		newStartLines.append(head + parts.first() + tail);
		startLines = newStartLines;
		last.lines = last.lines.mid(endIndex + 1);

		markDirty(startPage);
		markDirty(endPage);
		updateMaxLineLength(QStringList{ newStartLines.last() });
		refreshPageMemory(startPage);
		refreshPageMemory(endPage);
		invalidateCounts();
		enforceMemoryBudget(-1);
		return true;
	}

	newStartLines.append(head + parts.first());
	for (int i = 1; i < parts.size() - 1; i++) {
		newStartLines.append(parts.at(i));
	}
	startLines = newStartLines;

	QStringList newLastLines;
	newLastLines.append(parts.last() + tail);
	const QStringList rest = last.lines.mid(endIndex + 1);
	for (const QString& line : rest) {
		newLastLines.append(line);
	}
	last.lines = newLastLines;

	markDirty(startPage);
	markDirty(endPage);
	updateMaxLineLength(newStartLines.mid(startIndex));
	updateMaxLineLength(newLastLines);
	refreshPageMemory(startPage);
	refreshPageMemory(endPage);
	invalidateCounts();
	enforceMemoryBudget(-1);
	return true;
}

int PagedDocumentBuffer::pageCount() const {
	return pages.size();
}

QVector<int> PagedDocumentBuffer::pagesForLines(int startLine, int endLine) const {
	QVector<int> result;
	const int total = lineCount();
	if (total <= 0) {
		return result;
	}

	if (startLine > endLine) {
		std::swap(startLine, endLine);
	}
	startLine = qBound(0, startLine, total - 1);
	endLine = qBound(startLine, endLine, total - 1);

	int startPage = 0;
	int startIndex = 0;
	int endPage = 0;
	int endIndex = 0;
	if (!locate(startLine, startPage, startIndex)
		|| !locate(endLine, endPage, endIndex)) {
		return result;
	}

	for (int page = startPage; page <= endPage; page++) {
		result.append(page);
	}
	return result;
}

PagedPageSnapshot PagedDocumentBuffer::pageSnapshot(int page) const {
	PagedPageSnapshot snapshot;
	if (page < 0 || page >= pages.size()) {
		return snapshot;
	}

	const Page& source = pages.at(page);
	if (!source.dirty) {
		// Чистая страница восстанавливается чтением из файла.
		return snapshot;
	}

	QStringList lines;
	if (source.loaded) {
		lines = source.lines;
	}
	else if (source.spillOffset >= 0 && spillStore) {
		// Страница выгружена: для истории её содержимое нужно вернуть,
		// иначе отмена не сможет восстановить текст.
		if (!spillStore->read(source.spillOffset, source.spillBytes, lines)) {
			lines.clear();
		}
	}

	snapshot.lines = lines;
	snapshot.newline = source.newline;
	snapshot.loaded = true;
	snapshot.dirty = true;
	return snapshot;
}

void PagedDocumentBuffer::restorePage(int page, const PagedPageSnapshot& snapshot) {
	if (page < 0 || page >= pages.size()) {
		return;
	}

	Page& target = pages[page];
	if (snapshot.dirty) {
		if (!target.dirty) {
			dirtyPages++;
		}
		target.lines = snapshot.lines;
		target.newline = snapshot.newline;
		target.loaded = true;
		target.dirty = true;
		target.spillOffset = -1;
		target.spillBytes = 0;
		refreshPageMemory(page);
	}
	else {
		if (target.dirty) {
			dirtyPages--;
		}
		target.lines.clear();
		target.newline = defaultNewline;
		target.loaded = false;
		target.dirty = false;
		target.spillOffset = -1;
		target.spillBytes = 0;
		target.currentCount = target.originalCount;
		refreshPageMemory(page);
	}

	invalidateCounts();
}

void PagedDocumentBuffer::markDirty(int page) {
	Page& target = pages[page];
	if (!target.dirty) {
		target.dirty = true;
		dirtyPages++;
	}
}

void PagedDocumentBuffer::refreshPageMemory(int pageIndex) const {
	Page& page = pages[pageIndex];
	if (!page.dirty || !page.loaded) {
		dirtyMemory -= page.memoryBytes;
		page.memoryBytes = 0;
		return;
	}

	const qint64 bytes = memoryOf(page.lines);
	dirtyMemory += bytes - page.memoryBytes;
	page.memoryBytes = bytes;
	page.currentCount = page.lines.size();
}

void PagedDocumentBuffer::updateMaxLineLength(const QStringList& lines) {
	for (const QString& line : lines) {
		maxLength = qMax(maxLength, line.size());
	}
}

int PagedDocumentBuffer::dirtyPageCount() const {
	return dirtyPages;
}

bool PagedDocumentBuffer::hasChanges() const {
	return dirtyPages > 0;
}

void PagedDocumentBuffer::setMemoryBudget(qint64 bytes) {
	budget = qMax<qint64>(0, bytes);
	enforceMemoryBudget();
}

qint64 PagedDocumentBuffer::memoryBudget() const {
	return budget;
}

qint64 PagedDocumentBuffer::bufferedBytes() const {
	return dirtyMemory;
}

int PagedDocumentBuffer::spilledPageCount() const {
	int count = 0;
	for (const Page& page : pages) {
		// Страница может быть уже возвращена в память, но запись в файле
		// подкачки у неё осталась — для пользователя это та же выгрузка.
		if (page.dirty && page.spillOffset >= 0) {
			count++;
		}
	}
	return count;
}

QString PagedDocumentBuffer::spillFilePath() const {
	return spillStore ? spillStore->path() : QString();
}

void PagedDocumentBuffer::enforceMemoryBudget() const {
	enforceMemoryBudget(-1);
}

void PagedDocumentBuffer::enforceMemoryBudget(int protectedPage) const {
	if (budget <= 0 || dirtyMemory <= budget) {
		return;
	}

	// Сначала выгружаются страницы, к которым дольше всего не обращались.
	QVector<QPair<quint64, int>> candidates;
	candidates.reserve(pages.size());
	for (int i = 0; i < pages.size(); i++) {
		const Page& page = pages.at(i);
		if (page.dirty && page.loaded && i != protectedPage) {
			candidates.append({ page.access, i });
		}
	}
	std::sort(candidates.begin(), candidates.end());

	for (const auto& candidate : candidates) {
		if (dirtyMemory <= budget) {
			break;
		}
		spillPage(candidate.second);
	}
}

bool PagedDocumentBuffer::spillPage(int pageIndex) const {
	Page& page = pages[pageIndex];
	if (!page.loaded || !page.dirty) {
		return false;
	}

	if (!spillStore) {
		spillStore = std::make_shared<PagedSpillStore>();
	}
	if (!spillStore->isValid()) {
		// Нет временного файла — оставляем страницу в памяти: терять
		// данные из-за недоступного каталога нельзя.
		return false;
	}

	qint64 offset = -1;
	qint64 size = 0;
	if (!spillStore->write(page.lines, offset, size)) {
		return false;
	}

	page.spillOffset = offset;
	page.spillBytes = size;
	page.lines.clear();
	page.loaded = false;
	dirtyMemory -= page.memoryBytes;
	page.memoryBytes = 0;
	return true;
}

bool PagedDocumentBuffer::saveTo(const QString& targetPath, QString* error,
	const std::function<QString(const QString&)>& lineTransform) const {
	if (lineTransform) {
		return saveWithTransform(targetPath, error, lineTransform);
	}

	if (targetPath.isEmpty()) {
		if (error) {
			*error = QStringLiteral("Не задан путь для сохранения");
		}
		return false;
	}

	QFile source(filePath);
	const bool sourceOpen = !filePath.isEmpty() && source.open(QIODevice::ReadOnly);

	QSaveFile output(targetPath);
	if (!output.open(QIODevice::WriteOnly)) {
		if (error) {
			*error = QStringLiteral("Не удалось открыть файл для записи: %1")
				.arg(output.errorString());
		}
		return false;
	}

	QTextCodec* codec = codecForName(codecName);
	if (!codec) {
		if (error) {
			*error = QStringLiteral("Не удалось подобрать кодировку: %1")
				.arg(QString::fromLatin1(codecName));
		}
		output.cancelWriting();
		return false;
	}

	ensurePrefix();
	const int totalLines = prefix.isEmpty() ? 0 : prefix.last();
	const QByteArray newlineFallback = codec->fromUnicode(defaultNewline);

	QByteArray copyBuffer;
	copyBuffer.resize(static_cast<int>(kCopyChunk));
	bool failed = false;

	for (int pageIndex = 0; pageIndex < pages.size() && !failed; pageIndex++) {
		const Page& page = pages.at(pageIndex);
		const bool hasFollowing = prefix.value(pageIndex + 1) < totalLines;

		if (!page.dirty) {
			if (page.originalCount <= 0) {
				continue;
			}
			if (!sourceOpen) {
				if (!filePath.isEmpty()) {
					// Файл-источник не открылся: копировать неизменённые
					// страницы неоткуда, и молча потерять их нельзя.
					failed = true;
					break;
				}
				// Новый документ без файла: исходных байтов нет.
				continue;
			}

			const qint64 begin = lineStarts.value(page.originalFirst, 0);
			const int afterLast = page.originalFirst + page.originalCount;
			const qint64 end = afterLast < lineStarts.size()
				? lineStarts.at(afterLast) : fileSize;
			qint64 left = qMax<qint64>(0, end - begin);
			if (!source.seek(begin)) {
				failed = true;
				break;
			}

			while (left > 0) {
				const qint64 read = source.read(copyBuffer.data(),
					qMin<qint64>(left, copyBuffer.size()));
				if (read <= 0) {
					break;
				}
				if (output.write(copyBuffer.constData(), read) != read) {
					failed = true;
					break;
				}
				left -= read;
			}
			if (left > 0) {
				failed = true;
			}
			continue;
		}

		QStringList lines;
		if (page.loaded) {
			lines = page.lines;
		}
		else if (page.spillOffset >= 0 && spillStore) {
			if (!spillStore->read(page.spillOffset, page.spillBytes, lines)) {
				failed = true;
				break;
			}
		}
		else {
			// Изменённая страница ни в памяти, ни в файле подкачки —
			// это ошибка состояния, писать пустоту нельзя.
			failed = true;
			break;
		}

		if (pageIndex == 0 && bom && codecName == "UTF-8") {
			if (output.write("\xEF\xBB\xBF", 3) != 3) {
				failed = true;
				break;
			}
		}

		const QByteArray newline = page.newline.isEmpty()
			? newlineFallback : codec->fromUnicode(page.newline);
		for (int i = 0; i < lines.size(); i++) {
			const QByteArray encoded = codec->fromUnicode(lines.at(i));
			if (output.write(encoded) != encoded.size()) {
				failed = true;
				break;
			}
			const bool lastLine = i + 1 == lines.size();
			if (lastLine && !hasFollowing) {
				continue;
			}
			if (output.write(newline) != newline.size()) {
				failed = true;
				break;
			}
		}
	}

	// Исходный файл нужно закрыть до commit(): QSaveFile переименовывает
	// временный файл поверх целевого, а на Windows открытый файл этому мешает.
	source.close();

	if (!failed && !output.commit()) {
		failed = true;
	}

	if (failed) {
		if (error) {
			*error = QStringLiteral("Не удалось записать файл: %1")
				.arg(output.errorString());
		}
		output.cancelWriting();
		return false;
	}

	return true;
}

bool PagedDocumentBuffer::saveWithTransform(const QString& targetPath,
	QString* error,
	const std::function<QString(const QString&)>& lineTransform) const {
	if (!lineTransform) {
		if (error) {
			*error = QStringLiteral("Не задано преобразование строк");
		}
		return false;
	}
	if (targetPath.isEmpty()) {
		if (error) {
			*error = QStringLiteral("Не задан путь для сохранения");
		}
		return false;
	}

	QSaveFile output(targetPath);
	if (!output.open(QIODevice::WriteOnly)) {
		if (error) {
			*error = QStringLiteral("Не удалось открыть файл для записи: %1")
				.arg(output.errorString());
		}
		return false;
	}

	QTextCodec* codec = codecForName(codecName);
	if (!codec) {
		if (error) {
			*error = QStringLiteral("Не удалось подобрать кодировку: %1")
				.arg(QString::fromLatin1(codecName));
		}
		output.cancelWriting();
		return false;
	}

	ensurePrefix();
	const int totalLines = prefix.isEmpty() ? 0 : prefix.last();
	bool failed = false;

	if (bom && codecName == "UTF-8" && totalLines > 0) {
		if (output.write("\xEF\xBB\xBF", 3) != 3) {
			failed = true;
		}
	}

	const QByteArray newline = codec->fromUnicode(defaultNewline);
	for (int line = 0; line < totalLines && !failed; line++) {
		const QString converted = lineTransform(lineAt(line));
		const QByteArray encoded = codec->fromUnicode(converted);
		if (output.write(encoded) != encoded.size()) {
			failed = true;
			break;
		}
		if (line + 1 < totalLines && output.write(newline) != newline.size()) {
			failed = true;
			break;
		}
	}

	if (!failed && !output.commit()) {
		failed = true;
	}

	if (failed) {
		if (error) {
			*error = QStringLiteral("Не удалось записать файл: %1")
				.arg(output.errorString());
		}
		output.cancelWriting();
		return false;
	}

	return true;
}
