#include "Content/SystemRouter/models/system_router_mime.h"

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

namespace SystemRouterMime {

bool isCodeEditorProperty(const QStringList& mimeTypes) {
	const bool command = hasMime(mimeTypes, kCommand);
	const bool parameter = hasMime(mimeTypes, kCommandParameter);
	const bool context = hasMime(mimeTypes, kContextVariable);

	// В code-editor открывается специальный параметр команды, помеченный
	// одновременно как команда, параметр команды или контекстная переменная.
	return command || parameter || context;
}

} // namespace SystemRouterMime
