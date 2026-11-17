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
	void testContext_VariablesLazyMode();
	void testContext_VariablesMutableMode();
	void testContext_VariablesReadOnlyMode();
	void testContext_VariablesAtomicMode();
	void testContext_ScopeIsolation();
	void testContext_BootstrapContextBuilder();
	void testContext_ChildContexts();
	void testContext_PruneDeadContexts();
	void testContext_NameSpacesAliveOnly();
	void testContext_ChildContextsReturnOnlyAlive();
	void testContext_CleanupRunsInBackground();
	void testContext_BootstrapMigrations();
	void testContext_BootstrapMigrationErrors();

	// test_command_parser_command
	void testParser_ParseCommand_ShouldReturnNameAndParameters();
	void testParser_ParseCommand_ShouldParseQuotedAndBlockValues();
	void testParser_ParseCommand_ShouldRejectLegacyBlockValues();
	void testParser_ParseCommand_ShouldFailWithoutEquals();
	void testParser_ParseCommand_ShouldRequireCommaSeparator();
	void testCommandProcessor_Execute_CreatesMutableCommandContext();
	void testCommandProcessor_Execute_KeepsParentVariables();

	// palette
	void testPalette_ParsePalette_ShouldReturnPaletteOptions();
};
