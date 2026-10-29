#include "Libs/Engine/scripting/parser/stylesheets/parser_palette_rule.h"
#include "Libs/Engine/scripting/parser/base/parser_identifier.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"
#include <list>


namespace {
static std::list<QString> _colorGroups = {
	"Active", "Disabled", "Inactive", "NColorGroups", "Current", "All", "Normal"
};
static std::list<QString> _colorRoles = {
	"WindowText", "Button", "Light", "Midlight", "Dark", "Mid",
	"Text", "BrightText", "ButtonText", "Base", "Window", "Shadow",
	"Highlight", "HighlightedText",
	"Link", "LinkVisited",
	"AlternateBase",
	"NoRole",
	"ToolTipBase", "ToolTipText",
	"PlaceholderText",
	"Accent"
};
}

ParserPaletteRule::~ParserPaletteRule() = default;

QString ParserPaletteRule::name() const {
	return "ParserPaletteRule";
}

IAstPtr ParserPaletteRule::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto rule = ParserIdentifier().parse(cursor, error);
	if (!rule || error) {
		return nullptr;
	}

	auto startLoc = cursor.currentLocation();
	auto node = std::make_shared<AstBase>();
	node->setLocation(startLoc);
	backup.commit();
	return node;
}


