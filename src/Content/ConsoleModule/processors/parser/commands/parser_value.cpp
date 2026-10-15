#include "Content/ConsoleModule/processors/parser/commands/parser_value.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_value.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QDomDocument>

class ParserValue::Private {
public:
	Private(ParserValue* parent) : q(parent) { }
	ParserValue* q;

	IAstPtr tryReadQuotedText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadMultilineText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadJsonText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadXmlText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadIniText(SourceCursor& cursor, std::optional<ParseError>& error);
};

ParserValue::ParserValue()
	: d(std::make_unique<Private>(this)) {
}

ParserValue::~ParserValue() = default;


QString ParserValue::name() const {
	return "ParserValue";
}

IAstPtr ParserValue::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	if (auto ast = d->tryReadQuotedText(cursor, error)) {
		backup.commit();
		return ast;
	}

	if (auto ast = d->tryReadMultilineText(cursor, error)) {
		backup.commit();
		return ast;
	}

	if (auto ast = d->tryReadJsonText(cursor, error)) {
		backup.commit();
		return ast;
	}

	if (auto ast = d->tryReadXmlText(cursor, error)) {
		backup.commit();
		return ast;
	}

	if (auto ast = d->tryReadIniText(cursor, error)) {
		backup.commit();
		return ast;
	}

	return nullptr;
}

IAstPtr ParserValue::Private::tryReadIniText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<ini>>", "<</ini>>", true, "Read json application/json", error);
	if (result.isEmpty()) {
		return nullptr;
	}

	QMap<QString, QVariant> iniMap;
	const auto lines = result.split('\n', Qt::SkipEmptyParts);
	for (const auto& iniLine : lines) {
		const int eqPos = iniLine.indexOf('=');
		if (eqPos > 0) {
			const auto key = iniLine.left(eqPos).trimmed();
			const auto value = iniLine.mid(eqPos + 1).trimmed();
			iniMap.insert(key.toUtf8(), value.toUtf8());
		}
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", QVariant::fromValue(iniMap));
	node->setParameter("mime-type", "text/x-ini");
	return node;
}

IAstPtr ParserValue::Private::tryReadXmlText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<xml>>", "<</xml>>", true, "Read json application/json", error);
	if (result.isEmpty()) {
		return nullptr;
	}

	QDomDocument doc;
	if (!doc.setContent(result)) {
		error = ParseError(startLoc, "Invalid XML.");
		return nullptr;
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", QVariant::fromValue(doc));
	node->setParameter("mime-type", "application/xml");
	return node;
}

IAstPtr ParserValue::Private::tryReadJsonText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<json>>", "<</json>>", true, "Read json application/json", error);
	if (result.isEmpty()) {
		return nullptr;
	}

	QJsonParseError jsonError;
	QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &jsonError);
	if (jsonError.error != QJsonParseError::NoError) {
		error = ParseError(startLoc, QString("Invalid JSON: %1.")
			.arg(jsonError.errorString()));
		return nullptr;
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", QVariant::fromValue(doc));
	node->setParameter("mime-type", "application/json");
	return node;
}

IAstPtr ParserValue::Private::tryReadMultilineText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<text>>", "<</text>>", true, "Read multiline text/plane", error);
	if (result.isEmpty()) {
		return nullptr;
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", result.toUtf8());
	node->setParameter("mime-type", "text/plane");
	return node;
}


IAstPtr ParserValue::Private::tryReadQuotedText(SourceCursor& cursor, std::optional<ParseError>& error) {
	if (cursor.peek() != '"') {
		return nullptr;
	}

	auto startLoc = cursor.currentLocation();
	cursor.advance();

	QString result;
	bool escaped = false;

	while (!cursor.isEnd()) {
		QChar ch = cursor.peek();

		if (escaped) {
			// Обработка экранированных символов
			switch (ch.unicode()) {
			case 'n': result += '\n'; break;
			case 't': result += '\t'; break;
			case 'r': result += '\r'; break;
			case '"': result += '"'; break;
			case '\\': result += '\\'; break;
			default: result += ch; break;
			}
			cursor.advance();
			escaped = false;
		}
		else if (ch == '\\') {
			escaped = true;
			cursor.advance();
		}
		else if (ch == '"') {
			// Конец строки
			cursor.advance(); // Съедаем закрывающую "
			auto node = std::make_shared<AstValue>();
			node->setLocation(startLoc);
			node->setParameter("value", result);
			node->setParameter("mime-type", "text/plane");
			return node;
		}
		else if (ch == '\n') {
			// Перенос строки внутри строки без экранирования - ошибка
			error = ParseError{ cursor.currentLocation(), "Line break within a string (not multiline) literal" };
			return nullptr;
		}
		else {
			result += ch;
			cursor.advance();
		}
	}

	error = ParseError{ cursor.currentLocation(), "Expected end of string" };
	return nullptr;
}
