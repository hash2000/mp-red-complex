#pragma once
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"

/// Метаданные документа редактора кода: обязательные переменные контекста.
/// Регистрируется в WidgetRegistry для класса CodeEditorDocument.
class CodeEditorDocumentMetadata final : public IDocumentMetadata {
public:
	QList<DocumentVariable> variables() const override;
	/// Подсказка вкладки — путь к открытому файлу.
	QString tooltipVariable() const override;
};
