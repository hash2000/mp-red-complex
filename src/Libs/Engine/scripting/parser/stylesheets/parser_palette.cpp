#include "Libs/Engine/scripting/parser/stylesheets/parser_palette.h"
#include "Libs/Engine/scripting/parser/stylesheets/parser_palette_rule.h"
#include "Libs/Engine/scripting/parser/base/parser_variable.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

class ParserPalette::Private {
public:
	Private(ParserPalette* parent) : q(parent) { }
	ParserPalette* q;

	bool tryPopulateVariable(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulatePaletteRule(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error);
};

ParserPalette::ParserPalette()
	: d(std::make_unique<Private>(this)) {
}

ParserPalette::~ParserPalette() = default;

QString ParserPalette::name() const {
	return "ParserPalette";
}

IAstPtr ParserPalette::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	auto node = std::make_shared<AstBase>();
	node->setLocation(cursor.currentLocation());

	while (!cursor.isEnd()) {
		if (!d->tryPopulateVariable(node, cursor, error)) {
			return nullptr;
		}

		if (!d->tryPopulatePaletteRule(node, cursor, error)) {
			return nullptr;
		}

		return nullptr;
	}

	backup.commit();
	return node;
}

bool ParserPalette::Private::tryPopulatePaletteRule(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto rule = ParserPaletteRule().parse(cursor, error);
	if (error) {
		return false;
	}

	if (rule) {
		node->setChild("rules", rule);
	}

	return true;
}

bool ParserPalette::Private::tryPopulateVariable(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto variable = ParserVariable().parse(cursor, error);
	if (error) {
		return false;
	}

	if (variable) {
		node->setChild("variables", variable);
	}

	return true;
}
