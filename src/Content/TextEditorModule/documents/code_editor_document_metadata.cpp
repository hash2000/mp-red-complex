#include "Content/TextEditorModule/documents/code_editor_document_metadata.h"

QList<DocumentVariable> CodeEditorDocumentMetadata::variables() const {
	// Путь к файлу: редактор открывает его при привязке контекста.
	return {
		{ QStringLiteral("path"), QString::fromLatin1(DocumentVariableMime::kFilePath) }
	};
}

QString CodeEditorDocumentMetadata::tooltipVariable() const {
	return QStringLiteral("path");
}
