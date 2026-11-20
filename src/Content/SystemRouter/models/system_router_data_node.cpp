#include "Content/SystemRouter/models/system_router_data_node.h"
#include "Content/SystemRouter/models/system_router_mime.h"
#include "Libs/BaseWidgets/properties/property_mime.h"

namespace {

bool hasMime(const QStringList& mimeTypes, const char* mimeType) {
	for (const auto& raw : mimeTypes) {
		if (raw.trimmed().compare(QLatin1String(mimeType), Qt::CaseInsensitive) == 0) {
			return true;
		}
	}

	return false;
}

} // namespace

bool SystemRouterDataNode::propertyValueIsBlob(const QString& name) const {
	const auto mimeTypes = propertyMimeTypes(name);
	const bool command = hasMime(mimeTypes, SystemRouterMime::kCommand);
	const bool parameter = hasMime(mimeTypes, SystemRouterMime::kCommandParameter);
	const bool contextVariable = hasMime(mimeTypes, SystemRouterMime::kContextVariable);

	// Ссылка на команду — GUID; её место в value. Скрипт команды лежит
	// в data_blobs и редактируется code-editor по этому же GUID.
	if (command) {
		return false;
	}

	// Параметры и контекстные переменные и крупные текстовые значения
	// храним отдельно, чтобы не ломать JSON свойств кавычками и переносами.
	if (parameter || contextVariable) {
		return true;
	}

	return hasMime(mimeTypes, PropertyMimeType::kMultiline)
		|| hasMime(mimeTypes, PropertyMimeType::kJson)
		|| hasMime(mimeTypes, PropertyMimeType::kXml)
		|| hasMime(mimeTypes, "text/x-ini");
}
