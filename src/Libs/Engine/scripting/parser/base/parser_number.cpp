#include "Libs/Engine/scripting/parser/base/parser_number.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

#include <QRegularExpression>

ParserNumber::~ParserNumber() = default;


QString ParserNumber::name() const {
	return "ParserNumber";
}

/// @brief Читает числовое значение с опциональным знаком, разделителями разрядов (_) и дробной частью.
/// @return Найденное число как QStringView. Пустая строка, если совпадения нет.
/// @note Поддерживает форматы: 
///       - Целые: 123, +456, -789, 1_000, 1_000_000
///       - Дробные: 3.14, -0.5, 1_000.50, +1_234.567
IAstPtr ParserNumber::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

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
		qWarning() << "ParserNumber::tryReadNumberValue" << cursor.currentLocation().offset;
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(startLoc);
	node->setParameter("value", value);
	node->setParameter("type", "number");
	return node;
}


