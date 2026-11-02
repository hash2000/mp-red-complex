#pragma once

#include <QtTest/QtTest>

class TestEngine : public QObject {
	Q_OBJECT

private slots:
	// test_command_parsers
	void testParser_SourceCursorReadUntil_ShouldReturnTextBetweenQuotesMarkers();
	void testParser_ParseAttributesCollection_ShouldReturnAttribetes();
	void testParser_ParseInclude_ShouldReturnIncludeAstWithPath();
	void testParser_ParseSingleTextValue_ShouldReturnAstValue();
	void testParser_ParseMultilineTextValue_ShouldReturnAstValue();
	void testParser_ParseVariable_ShouldCheckRValue_data();
	void testParser_ParseVariable_ShouldCheckRValue();
	void testParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue();
	void testParser_ParseTypedVariableWithValue_ShouldCompareLTypeAndRType();
	void testParser_ParseTypedVariableWithValue_ShouldRequridTypeMustCorrecponded_data();
	void testParser_ParseTypedVariableWithValue_ShouldRequridTypeMustCorrecponded();
	void testParser_ParseRValueTypedVariable_ShouldReturnVariableDeclarationWithValue();
	void testParser_ParseVariableWithBinding_ShouldReturnVariableDeclarationWithBindName();
	void testParser_ParseBoolValue_ShouldReturnTrueFalseValue_data();
	void testParser_ParseBoolValue_ShouldReturnTrueFalseValue();
	void testParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber_data();
	void testParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber();
	void testParser_ParseNamespace_ShouldReturnNamespacePath();

	// test_command_context
	void testContext_BuildChildContext();

	// palette
	void testPalette_ParsePalette_ShouldReturnPaletteOptions();
};
