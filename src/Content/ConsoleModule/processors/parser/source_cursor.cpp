#include "Content/ConsoleModule/processors/parser/source_cursor.h"

SourceCursor::SourceCursor(QStringView text)
	: _text(text)
	, _pos(0)
	, _line(1)
	, _column(1) {
}

// Текущий символ (или QChar::Null если конец)
QChar SourceCursor::peek() const {
	return _pos < _text.length() ? _text[_pos] : QChar::Null;
}

// Получить символ и сдвинуть курсор
QChar SourceCursor::advance() {
	if (_pos >= _text.length()) {
		return QChar::Null;
	}

	QChar ch = _text[_pos++];
	if (ch == '\n') {
		_line++;
		_column = 1;
	}
	else {
		_column++;
	}
	return ch;
}

QChar SourceCursor::advance(size_t len) {
	if (len == 0) {
		return peek();
	}
	QChar lastChar = QChar::Null;
	for (size_t i = 0; i < len; ++i) {
		if (_pos >= _text.length()) {
			// Достигли конца текста, дальше двигаться некуда.
			// Возвращаем Null, чтобы сигнализировать о неполном продвижении / EOF.
			return QChar::Null;
		}
		// Переиспользуем одиночный advance(), чтобы корректно 
		// обновить _line и _column при встрече с '\n'
		lastChar = advance();
	}

	return lastChar;
}

QStringView SourceCursor::readUntil(const QString& endMarker, bool multiline) {
	int start = _pos;
	int markerLen = endMarker.length();

	if (markerLen == 0) {
		return QStringView(); // Защита от пустого маркера
	}

	// Безопасное условие: проверяем, что до конца строки точно поместится маркер
	while (_pos + markerLen <= _text.length()) {

		// 1. Если multiline = false, и мы уперлись в конец строки, прекращаем поиск.
		// Курсор остается на символе '\n', чтобы вызывающий код мог его корректно обработать.
		if (!multiline && _text[_pos] == '\n') {
			break;
		}

		// 2. Проверяем посимвольное совпадение с endMarker
		bool match = true;
		for (int i = 0; i < markerLen; i++) {
			if (_text[_pos + i] != endMarker[i]) {
				match = false;
				break;
			}
		}

		if (match) {
			// Маркер найден! Формируем результат (текст ДО маркера)
			QStringView result = _text.mid(start, _pos - start);

			// Продвигаем курсор через сам маркер, корректно обновляя координаты
			for (int i = 0; i < markerLen; i++) {
				if (_text[_pos] == '\n') {
					_line++;
					_column = 1;
				}
				else {
					_column++;
				}
				_pos++;
			}
			return result;
		}

		// 3. Совпадения нет, продвигаемся на 1 символ вперед
		if (_text[_pos] == '\n') {
			_line++;
			_column = 1;
		}
		else {
			_column++;
		}
		_pos++;
	}

	// Если цикл завершился, а маркер не найден:
	// Возвращаем всё, что удалось накопить до текущей позиции (или до конца текста)
	return _text.mid(start, _pos - start);
}

// Пропустить пробельные символы и комментарии (одной строки //)
bool SourceCursor::skipWhitespaceAndComments(std::optional<ParseError>& error) {
	while (_pos < _text.length()) {
		QChar ch = peek();

		// 1. Обычные пробельные символы (пробел, табуляция, \n, \r)
		if (ch.isSpace()) {
			advance();
		}
		// 2. Однострочный комментарий //
		else if (ch == '/' && _pos + 1 < _text.length() && _text[_pos + 1] == '/') {
			while (_pos < _text.length() && peek() != '\n') {
				advance();
			}
		}
		// 3. Многострочный комментарий /* */
		else if (ch == '/' && _pos + 1 < _text.length() && _text[_pos + 1] == '*') {
			advance(); // съедаем '/'
			advance(); // съедаем '*'

			// Ищем закрывающую пару */
			bool commentClosed = false;
			while (_pos < _text.length()) {
				if (peek() == '*' && _pos + 1 < _text.length() && _text[_pos + 1] == '/') {
					advance(); // съедаем '*'
					advance(); // съедаем '/'
					commentClosed = true;
					break; // Комментарий успешно закрыт, выходим из внутреннего цикла
				}
				// Просто двигаемся дальше. advance() корректно обработает \n внутри комментария
				advance();
			}

			// Опционально: обработка незакрытого комментария
			if (!commentClosed) {
				error = ParseError(currentLocation(), "Expected end of multiline comment");
				return false;
			}
		}
		// 4. Наткнулись на значимый символ (код, идентификатор и т.д.)
		else {
			break;
		}
	}

	return true;
}

// Прочитать последовательность символов, удовлетворяющих условию
QStringView SourceCursor::readWhile(bool (*condition)(QChar)) {
	int start = _pos;
	while (_pos < _text.length() && condition(_text[_pos])) {
		advance();
	}
	return _text.mid(start, _pos - start);
}

// Проверка, начинается ли текст с определенной подстроки (без сдвига курсора)
bool SourceCursor::startsWith(QStringView prefix) const {
	return _text.mid(_pos, prefix.length()) == prefix;
}

SourceLocation SourceCursor::currentLocation() const {
	return { _pos, _line, _column };
}

bool SourceCursor::isEnd() const {
	return _pos >= _text.length();
}

void SourceCursor::resetPosition(const SourceCursor& src) {
	_pos = src._pos;
	_line = src._line;
	_column = src._column;
}

bool SourceCursor::token(const QString& t) {
	if (!startsWith(t)) {
		return false;
	}
	advance(t.length());
	return true;
}

bool SourceCursor::tryToken(const QString& t, const QString& errorSuffix, std::optional<ParseError>& error) {
	auto loc = currentLocation();
	if (!token(t)) {
		error = ParseError(loc, QString("Expected %1. %2")
			.arg(t)
			.arg(errorSuffix));
		return false;
	}
	return true;
}

QStringView SourceCursor::readQuotedBlock(const QString& startQuote, const QString& endQuote, bool multiline) {
	if (!startsWith(startQuote)) {
		return QStringView();
	}
	advance(startQuote.length());
	return readUntil(endQuote, multiline);
}

QStringView SourceCursor::readQuotedBlock(const QString& quote, bool multiline) {
	return readQuotedBlock(quote, quote, multiline);
}

QStringView SourceCursor::tryReadQuotedBlock(const QString& startQuote, const QString& endQuote, bool multiline, const QString& errorSuffix, std::optional<ParseError>& error) {
	auto loc = currentLocation();
	if (!startsWith(startQuote)) {
		error = ParseError(loc, QString("Missing start quote %1. %2")
			.arg(startQuote)
			.arg(errorSuffix));
		return QStringView();
	}
	advance(startQuote.length());
	loc = currentLocation();
	auto result = readUntil(endQuote, multiline);
	if (result.isEmpty()) {
		error = ParseError(loc, QString("Missing end quote %1. %2")
			.arg(startQuote)
			.arg(errorSuffix));
		return QStringView();
	}

	return result;
}

QStringView SourceCursor::tryReadQuotedBlock(const QString& quote, bool multiline, const QString& errorSuffix, std::optional<ParseError>& error) {
	return tryReadQuotedBlock(quote, quote, multiline, errorSuffix, error);
}
