#include "Content/SystemRouter/models/system_router_command.h"

#include <QUuid>

QString normalizeSystemRouterCommandId(const QString& raw) {
	const auto trimmed = raw.trimmed();
	if (trimmed.isEmpty()) {
		return {};
	}

	const auto uuid = QUuid::fromString(trimmed);
	if (!uuid.isNull()) {
		return uuid.toString(QUuid::WithoutBraces);
	}

	return trimmed;
}
