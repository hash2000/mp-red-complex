#include "Content/FetchApiModule/documents/fetch_api_document_metadata.h"

QList<DocumentVariable> FetchApiDocumentMetadata::variables() const {
	return {
		{ QStringLiteral("location"), { QString::fromLatin1(DocumentVariableMime::kText) } },
		{ QStringLiteral("method"), { QString::fromLatin1(DocumentVariableMime::kText) } },
		{ QStringLiteral("body"), { QString::fromLatin1(DocumentVariableMime::kMultiline) } },
		{ QStringLiteral("params"), { QString::fromLatin1(DocumentVariableMime::kMultiline) } },
		{ QStringLiteral("ssl-policy"), { QString::fromLatin1(DocumentVariableMime::kText) } },
		{ QStringLiteral("handler"), { QString::fromLatin1(DocumentVariableMime::kText) } },
	};
}

QString FetchApiDocumentMetadata::tooltipVariable() const {
	return QStringLiteral("url");
}
