#pragma once
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"

/// Метаданные документа FetchApi: переменные контекста, из которых документ
/// восстанавливает параметры запроса после открытия.
class FetchApiDocumentMetadata final : public IDocumentMetadata {
public:
	QList<DocumentVariable> variables() const override;
	/// Подсказка вкладки — фактический URL последнего ответа.
	QString tooltipVariable() const override;
};
