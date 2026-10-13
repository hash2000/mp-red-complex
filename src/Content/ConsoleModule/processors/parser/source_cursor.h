#pragma once
#include <QStringView>
#include <QString>
#include <optional>

struct SourceLocation {
	int offset; // Позиция в байтах/символах от начала
	int line;   // Номер строки (начинается с 1)
	int column; // Номер колонки (начинается с 1)
};

struct ParseError {
	SourceLocation location;
	QString message;
};

// Класс для безопасного и удобного перемещения по тексту
class SourceCursor {
public:
	explicit SourceCursor(QStringView text);

	// Текущий символ (или QChar::Null если конец)
	QChar peek() const;

	// Получить символ и сдвинуть курсор
	QChar advance();
	QChar advance(size_t len);

	// Пропустить пробельные символы и комментарии (// и /* */)
	bool skipWhitespaceAndComments(std::optional<ParseError>& error);

	// Прочитать последовательность символов, удовлетворяющих условию
	QStringView readWhile(bool (*condition)(QChar));

	// Проверка, начинается ли текст с определенной подстроки (без сдвига курсора)
	bool startsWith(QStringView prefix) const;

	// Прочитать курсор до указанного маркера
	QStringView readUntil(const QString& endMarker, bool multiline = true);

	SourceLocation currentLocation() const;

	bool isEnd() const;

	void resetPosition(const SourceCursor& src);

	bool token(const QString& t);
	bool tryToken(const QString& t, const QString& errorSuffix, std::optional<ParseError>& error);
	QStringView readQuotedBlock(const QString& startQuote, const QString& endQuote, bool multiline);
	QStringView readQuotedBlock(const QString& quote, bool multiline);
	QStringView tryReadQuotedBlock(const QString& startQuote, const QString& endQuote, bool multiline, const QString& errorSuffix, std::optional<ParseError>& error);
	QStringView tryReadQuotedBlock(const QString& quote, bool multiline, const QString& errorSuffix, std::optional<ParseError>& error);

private:
	QStringView _text;
	int _pos;
	int _line;
	int _column;
};

class SourceCursorTransaction final {
public:
	SourceCursorTransaction(SourceCursor& cursor)
		: _cursor(cursor)
		, _backup(cursor) {
	}

	~SourceCursorTransaction() {
		if (_commited) {
			return;
		}

		_cursor.resetPosition(_backup);
	}

	void commit() {
		_commited = true;
	}
private:
	SourceCursor& _cursor;
	SourceCursor _backup;
	bool _commited = false;
};
