#include "Content/ConsoleModule/processors/parser/commands/parser_value.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_value.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QDomDocument>
#include <QRegularExpression>

class ParserValue::Private {
public:
	Private(ParserValue* parent) : q(parent) { }
	ParserValue* q;
	AstDataType expectedType;
	QString expectedMimeType;

	IAstPtr tryReadQuotedText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadMultilineText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadJsonText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadXmlText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadIniText(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadBoolValue(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr tryReadNumberValue(SourceCursor& cursor, std::optional<ParseError>& error);

};

ParserValue::ParserValue(AstDataType expectedType, const QString& expectedMimeType)
	: d(std::make_unique<Private>(this)) {
	d->expectedType = expectedType;
	d->expectedMimeType = expectedMimeType;
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

	auto ast = d->tryReadQuotedText(cursor, error);
	if (error.has_value()) {
		return nullptr;
	}
	else if(ast) {
		backup.commit();
		return ast;
	}

	ast = d->tryReadMultilineText(cursor, error);
	if (error.has_value()) {
		return nullptr;
	}
	else if (ast) {
		backup.commit();
		return ast;
	}

	ast = d->tryReadJsonText(cursor, error);
	if (error.has_value()) {
		return nullptr;
	}
	else if (ast) {
		backup.commit();
		return ast;
	}

	ast = d->tryReadXmlText(cursor, error);
	if (error.has_value()) {
		return nullptr;
	}
	else if (ast) {
		backup.commit();
		return ast;
	}

	ast = d->tryReadIniText(cursor, error);
	if (error.has_value()) {
		return nullptr;
	}
	else if (ast) {
		backup.commit();
		return ast;
	}

	ast = d->tryReadBoolValue(cursor, error);
	if (error.has_value()) {
		return nullptr;
	}
	else if (ast) {
		backup.commit();
		return ast;
	}

	ast = d->tryReadNumberValue(cursor, error);
	if (error.has_value()) {
		return nullptr;
	}
	else if (ast) {
		backup.commit();
		return ast;
	}

	return nullptr;
}

/// @brief Читает числовое значение с опциональным знаком, разделителями разрядов (_) и дробной частью.
/// @return Найденное число как QStringView. Пустая строка, если совпадения нет.
/// @note Поддерживает форматы: 
///       - Целые: 123, +456, -789, 1_000, 1_000_000
///       - Дробные: 3.14, -0.5, 1_000.50, +1_234.567
IAstPtr ParserValue::Private::tryReadNumberValue(SourceCursor& cursor, std::optional<ParseError>& error) {
	// Регулярное выражение для числа:
	// ^[+-]?           - опциональный знак + или -
	// \d+              - одна или более цифр (целая часть)
	// (_\d+)*          - ноль или более групп из _ и цифр (разделители разрядов)
	// (\.\d+)?         - опциональная дробная часть: точка и одна или более цифр
	// (?![_\d])        - негативный lookahead: после числа не должно быть _ или цифры
	//                    (чтобы не парсить "123abc" как число "123")
	static const QRegularExpression identifierRegex(
		R"(^[+-]?\d+(_\d+)*(\.\d+)?(?![_\d]))"
	);

	auto startLoc = cursor.currentLocation();
	QStringView numView = cursor.readRegex(identifierRegex);
	if (numView.isEmpty()) {
		return nullptr;
	}

	auto cleanNumber = numView.toString();
	cleanNumber.remove('_');

	bool ok;
	auto value = cleanNumber.toDouble(&ok);

	if (!ok) {
		qWarning() << "ParserValue::tryReadNumberValue" << cursor.currentLocation().offset;
		return nullptr;
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", value);
	node->setParameter("type", QVariant::fromValue(static_cast<char>(AstDataType::Bool)));
	return node;

}

IAstPtr ParserValue::Private::tryReadBoolValue(SourceCursor& cursor, std::optional<ParseError>& error) {
	static QString trueToken = "true";
	static QString falseToken = "false";
	auto startLoc = cursor.currentLocation();
	bool result;
	if (cursor.token(trueToken)) {
		cursor.advance(trueToken.length());
		result = true;
	}
	else if (cursor.token(falseToken)) {
		cursor.advance(falseToken.length());
		result = false;
	}
	else {
		return nullptr;
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", QVariant::fromValue(result));
	node->setParameter("type", QVariant::fromValue(static_cast<char>(AstDataType::Number)));
	return node;
}

IAstPtr ParserValue::Private::tryReadIniText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<ini>>", "<</ini>>", true, "Read string text/x-ini", error);
	if (result.isEmpty()) {
		error = std::nullopt;
		return nullptr;
	}

	QMap<QString, QVariant> iniMap;
	const auto lines = result.split('\n', Qt::SkipEmptyParts);
	for (const auto& iniLine : lines) {
		const int eqPos = iniLine.indexOf('=');
		if (eqPos > 0) {
			const auto key = iniLine.left(eqPos).trimmed();
			const auto value = iniLine.mid(eqPos + 1).trimmed();
			iniMap.insert(key.toString(), value.toString());
		}
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", QVariant::fromValue(iniMap));
	node->setParameter("mime-type", "text/x-ini");
	node->setParameter("type", QVariant::fromValue(static_cast<char>(AstDataType::String)));
	return node;
}

IAstPtr ParserValue::Private::tryReadXmlText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<xml>>", "<</xml>>", true, "Read string application/xml", error);
	if (result.isEmpty()) {
		error = std::nullopt;
		return nullptr;
	}

	QDomDocument doc;
	const auto xmlDocResult = doc.setContent(result);
	if (!xmlDocResult) {
		error = ParseError(startLoc, QString("Invalid XML. %1 at line %2 col %3")
			.arg(xmlDocResult.errorMessage)
			.arg(xmlDocResult.errorLine)
			.arg(xmlDocResult.errorColumn));
		return nullptr;
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", QVariant::fromValue(doc));
	node->setParameter("mime-type", "application/xml");
	node->setParameter("type", QVariant::fromValue(static_cast<char>(AstDataType::String)));
	return node;
}

IAstPtr ParserValue::Private::tryReadJsonText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<json>>", "<</json>>", true, "Read string application/json", error);
	if (result.isEmpty()) {
		error = std::nullopt;
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
	node->setParameter("type", QVariant::fromValue(static_cast<char>(AstDataType::String)));
	return node;
}

IAstPtr ParserValue::Private::tryReadMultilineText(SourceCursor& cursor, std::optional<ParseError>& error) {
	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("<<text>>", "<</text>>", true, "Read multiline text/plane", error);
	if (result.isEmpty()) {
		error = std::nullopt;
		return nullptr;
	}

	auto node = std::make_shared<AstValue>();
	node->setLocation(startLoc);
	node->setParameter("value", result.toUtf8());
	node->setParameter("mime-type", "text/plane");
	node->setParameter("type", QVariant::fromValue(static_cast<char>(AstDataType::String)));
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
			node->setParameter("type", QVariant::fromValue(static_cast<char>(AstDataType::String)));
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
