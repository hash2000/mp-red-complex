#include "Content/DatabaseModule/documents/database_backup_document_metadata.h"

QList<DocumentVariable> DatabaseBackupDocumentMetadata::variables() const {
	return {
		{ QStringLiteral("store"), QString::fromLatin1(DocumentVariableMime::kText) },
	};
}

QString DatabaseBackupDocumentMetadata::tooltipVariable() const {
	return QStringLiteral("store");
}
