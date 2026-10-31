#include "test_command.h"
#include "Libs/Engine/scripting/parser/stylesheets/parser_palette.h"

void TestCommandModule::testPalette_ParsePalette_ShouldReturnPaletteOptions() {
	// Given
	QString testScript = R"(
processor application.palette {
	mapping states.ui[
		palette,
		name="dark"
		version="0.0.1",
	] {
		// Build an initial palette based 
		Extends				[ rgb														]:		(35, 35, 35)

		// List box backgrounds, text entry backgrounds, menu backgrounds
		Base					[ rgb,	darker=130							]:		(35, 35, 35)

		// Window colors
		Window				[ rgb,								Active		]:		(50, 50, 50)
		Window				[ rgb,								Inactive	]:		(40, 40, 40)
		Window				[ rgb,	darker=200,		Disabled	]:		(50, 50, 50)

		// Button text
		ButtonText		[ rgb,								Active		]:		(207, 207, 207)
		ButtonText		[ rgb,								Inactive	]:		(207, 207, 207)
		ButtonText		[ rgb,	darker=200,		Disabled	]:		(207, 207, 207)

		// WindowText is supposed to be against QPalette::Window
		WindowText		[ rgb,								Active		]:		(207, 207, 207)
		WindowText		[ rgb,								Inactive	]:		(207, 207, 207)
		WindowText		[ rgb,	darker=200,		Disabled	]:		(207, 207, 207)

		// Menu text, text edit text, table cell text
		Text					[ rgb,	darker=115,		Active		]:		(207, 207, 207)
		Text					[ rgb,	darker=115,		Inactive	]:		(207, 207, 207)
		Text					[ rgb,								Disabled	]:		(102, 102, 102)

		// Disabled menu item text shadow
		Light					[ rgb,	darker=200,		Disabled	]:		(35, 35, 35)

		// Highlight (selected list box row, selected grid cell background, selected tab text
		Highlight			[ rgb,								Active		]:		(62, 112, 205)
		Highlight			[ rgb,								Inactive	]:		(62, 112, 205)
		Highlight			[ rgb,								Disabled	]:		(62, 112, 205)
	}
}
)";

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserPalette().parse(source, error);

	//// Then
	//QCOMPARE(error.has_value(), false);
	//QVERIFY(result != nullptr);

	//auto location = result->location();
	//auto srcLocation = source.currentLocation();

}

