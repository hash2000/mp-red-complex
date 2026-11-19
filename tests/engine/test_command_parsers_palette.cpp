#include "test_command.h"

void TestEngine::testPalette_ParsePalette_ShouldReturnPaletteOptions() {
	// Given
	QString testScript = R"(
processor application.palette {
	mapping states.ui[
		palette,
		name="dark"
		version="0.0.1",
	] {
		// Build an initial palette based 
		vec[ rgb,							Extends										]	_:		(35, 35, 35)

		// List box backgrounds, text entry backgrounds, menu backgrounds
		vec[ rgb							Base											]	_:		(35, 35, 35)

		// Window colors
		vec[ rgb,							Window,					Active		]	_:		(50, 50, 50)
		vec[ rgb,							Window,					nactive		]	_:		(40, 40, 40)
		vec[ rgb,	darker=200, Window,					isabled		]	_:		(50, 50, 50)

		// Button text
		vec[ rgb,							ButtonText,			Active		]	_:		(207, 207, 207)
		vec[ rgb,							ButtonText,			Inactive	]	_:		(207, 207, 207)
		vec[ rgb, darker=200,	ButtonText,			Disabled	]	_:		(207, 207, 207)

		// WindowText is supposed to be against QPalette::Window
		vec[ rgb,							WindowText,			Active		]	_:		(207, 207, 207)
		vec[ rgb,							WindowText,			Inactive	]	_:		(207, 207, 207)
		vec[ rgb,							WindowText,			Disabled	]	_:		(207, 207, 207)

		// Menu text, text edit text, table cell text
		vec[ rgb,	darker=115,	Text,						Active		]	_:		(207, 207, 207)
		vec[ rgb,	darker=115,	Text,						Inactive	]	_:		(207, 207, 207)
		vec[ rgb,							Text,						Disabled	]	_:		(102, 102, 102)

		// Disabled menu item text shadow
		vec[ rgb,							Light,					Disabled	]	_:		(35, 35, 35)

		// Highlight (selected list box row, selected grid cell background, selected tab text
		vec[ rgb,	darker=200,	Highlight,			Active		]	_:		(62, 112, 205)
		vec[ rgb							Highlight,			Inactive	]	_:		(62, 112, 205)
		vec[ rgb							Highlight,			Disabled	]	_:		(62, 112, 205)
	}
}
)";

	//SourceCursor source(testScript);
	//std::optional<ParseError> error;

	//// When
	//auto result = ParserProcessor().parse(source, error);

	//// Then
	//QCOMPARE(error.has_value(), false);
	//QVERIFY(result != nullptr);

	//auto location = result->location();
	//auto srcLocation = source.currentLocation();

}

