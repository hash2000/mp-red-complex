#pragma once
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"

/// Метаданные документа резервного копирования: переменная `store`
/// восстанавливает выбранное хранилище после повторного открытия.
class DatabaseBackupDocumentMetadata final : public IDocumentMetadata {
public:
	QList<DocumentVariable> variables() const override;
	/// Подсказка вкладки — имя выбранного хранилища.
	QString tooltipVariable() const override;
};
