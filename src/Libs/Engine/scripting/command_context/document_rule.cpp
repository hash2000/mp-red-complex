#include "Libs/Engine/scripting/command_context/document_rule.h"

namespace {
constexpr const char* kDocumentRuleDocument = "document";
constexpr const char* kDocumentRuleSystemOptions = "system-options";
} // namespace

QString documentRuleName(DocumentRule rule) {
	switch (rule) {
	case DocumentRule::SystemOptions:
		return QString::fromLatin1(kDocumentRuleSystemOptions);
	case DocumentRule::Document:
		break;
	}

	return QString::fromLatin1(kDocumentRuleDocument);
}

std::optional<DocumentRule> documentRuleFromName(const QString& name) {
	if (name.compare(QLatin1String(kDocumentRuleSystemOptions), Qt::CaseInsensitive) == 0) {
		return DocumentRule::SystemOptions;
	}

	if (name.compare(QLatin1String(kDocumentRuleDocument), Qt::CaseInsensitive) == 0) {
		return DocumentRule::Document;
	}

	return std::nullopt;
}
