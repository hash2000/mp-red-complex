#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/models/system_router_mime.h"
#include "Libs/BaseWidgets/properties/property_mime.h"

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

namespace {

QString formatMimeType(const QStringList& mimeTypes) {
	for (const auto& raw : mimeTypes) {
		const auto mime = raw.trimmed().toLower();
		if (mime.isEmpty()
			|| mime == QLatin1String(SystemRouterMime::kCommand)
			|| mime == QLatin1String(SystemRouterMime::kCommandParameter)) {
			continue;
		}

		return mime;
	}

	return QString::fromLatin1(PropertyMimeType::kText);
}

bool hasMimeType(const QStringList& mimeTypes, const char* mimeType) {
	for (const auto& raw : mimeTypes) {
		if (raw.trimmed().compare(QLatin1String(mimeType), Qt::CaseInsensitive) == 0) {
			return true;
		}
	}

	return false;
}

QString quotedValue(const QString& value) {
	QString escaped = value;
	escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
	escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
	escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
	escaped.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
	escaped.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
	return QStringLiteral("\"%1\"").arg(escaped);
}

QString textValue(const QVariant& value) {
	const auto type = value.metaType().id();
	if (type == QMetaType::QVariantMap || type == QMetaType::QVariantList) {
		return QString::fromUtf8(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact));
	}
	if (type == QMetaType::QByteArray) {
		return QString::fromUtf8(value.toByteArray());
	}

	return value.toString();
}

QString jsonValue(const QVariant& value) {
	const auto type = value.metaType().id();
	QString result;
	if (type == QMetaType::QVariantMap || type == QMetaType::QVariantList) {
		result = QString::fromUtf8(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact));
	}
	else if (type == QMetaType::QString) {
		result = value.toString();
	}
	else {
		const auto json = QJsonValue::fromVariant(value);
		if (json.isObject()) {
			result = QString::fromUtf8(QJsonDocument(json.toObject()).toJson(QJsonDocument::Compact));
		}
		else if (json.isArray()) {
			result = QString::fromUtf8(QJsonDocument(json.toArray()).toJson(QJsonDocument::Compact));
		}
		else {
			result = value.toString();
		}
	}

	return result.isEmpty() ? QStringLiteral("\"\"") : result;
}

} // namespace

class SystemRouterCommandService::Private {
public:
	explicit Private(SystemRouterCommandService* parent)
		: q(parent) {
	}

	SystemRouterCommandService* q;
	ISystemRouterCommandsProvider* provider = nullptr;
	QString lastError;

	void clearError() {
		lastError.clear();
	}

	bool fail(const QString& message) {
		lastError = message;
		emit q->errorOccurred(message);
		return false;
	}
};

SystemRouterCommandService::SystemRouterCommandService(ISystemRouterCommandsProvider* provider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->provider = provider;
}

SystemRouterCommandService::~SystemRouterCommandService() = default;

ISystemRouterCommandsProvider* SystemRouterCommandService::dataProvider() const {
	return d->provider;
}

QList<SystemRouterCommand> SystemRouterCommandService::commands() const {
	if (!d->provider) {
		d->lastError = tr("Поставщик команд не задан");
		return {};
	}

	auto result = d->provider->commands();
	d->lastError = d->provider->lastError();
	return result;
}

std::optional<SystemRouterCommand> SystemRouterCommandService::command(const QString& id) const {
	if (!d->provider) {
		d->lastError = tr("Поставщик команд не задан");
		return std::nullopt;
	}

	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return std::nullopt;
	}

	auto result = d->provider->command(normalized);
	d->lastError = d->provider->lastError();
	return result;
}

bool SystemRouterCommandService::saveCommand(const SystemRouterCommand& command) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик команд не задан"));
	}

	const auto normalized = normalizeSystemRouterCommandId(command.id);
	if (normalized.isEmpty() || QUuid::fromString(normalized).isNull()) {
		return d->fail(tr("Идентификатор команды должен быть GUID"));
	}

	SystemRouterCommand stored = command;
	stored.id = normalized;
	stored.version = command.version > 0 ? command.version : 1;

	if (!d->provider->saveCommand(stored)) {
		return d->fail(d->provider->lastError());
	}

	return true;
}

bool SystemRouterCommandService::removeCommand(const QString& id) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик команд не задан"));
	}

	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return d->fail(tr("Идентификатор команды не задан"));
	}

	if (!d->provider->removeCommand(normalized)) {
		return d->fail(d->provider->lastError());
	}

	return true;
}

QList<SystemRouterScriptPreview> SystemRouterCommandService::previewsForNode(
	const LazyTreeNodePtr& node) const {
	QList<SystemRouterScriptPreview> result;
	if (!node) {
		return result;
	}

	const auto properties = node->properties();
	for (auto it = properties.constBegin(); it != properties.constEnd(); it++) {
		const auto mimeTypes = node->propertyMimeTypes(it.key());
		if (!hasMimeType(mimeTypes, SystemRouterMime::kCommand)) {
			continue;
		}

		SystemRouterScriptPreview preview;
		preview.propertyName = it.key();
		preview.commandId = normalizeSystemRouterCommandId(it.value().toString());

		if (preview.commandId.isEmpty()) {
			preview.error = tr("Ссылка на команду не задана");
			result.append(preview);
			continue;
		}

		if (!d->provider) {
			preview.error = tr("Поставщик команд не задан");
			result.append(preview);
			continue;
		}

		if (QUuid::fromString(preview.commandId).isNull()) {
			preview.error = tr("Некорректная ссылка на команду: %1").arg(preview.commandId);
			result.append(preview);
			continue;
		}

		const auto command = this->command(preview.commandId);
		if (!command.has_value()) {
			preview.error = tr("Команда не найдена: %1").arg(preview.commandId);
			result.append(preview);
			continue;
		}

		preview.command = *command;
		preview.commandFound = true;
		preview.renderedScript = renderScript(command->script, node,
			&preview.missingParameters);
		result.append(preview);
	}

	return result;
}

QString SystemRouterCommandService::renderScript(const QString& script,
	const LazyTreeNodePtr& node, QStringList* missingParameters) const {
	static const QRegularExpression expression(QStringLiteral(R"(\{\{\s*([^{}]+?)\s*\}\})"));

	QString result;
	int last = 0;
	QSet<QString> missing;

	auto iterator = expression.globalMatch(script);
	while (iterator.hasNext()) {
		const auto match = iterator.next();
		result += script.mid(last, match.capturedStart() - last);

		const auto name = match.captured(1).trimmed();
		bool found = false;
		if (node) {
			const auto properties = node->properties();
			const auto property = properties.constFind(name);
			if (property != properties.constEnd()
				&& hasMimeType(node->propertyMimeTypes(name), SystemRouterMime::kCommandParameter)) {
				result += formatParameterValue(node->propertyMimeTypes(name), property.value());
				found = true;
			}
		}

		if (!found) {
			result += QStringLiteral("<<undefined>>");
			if (!missing.contains(name)) {
				missing.insert(name);
				if (missingParameters) {
					missingParameters->append(name);
				}
			}
		}

		last = match.capturedEnd();
	}

	result += script.mid(last);
	return result;
}

QString SystemRouterCommandService::formatParameterValue(const QStringList& mimeTypes,
	const QVariant& value) {
	const auto mime = formatMimeType(mimeTypes);

	if (mime == QLatin1String(PropertyMimeType::kBoolean)) {
		return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
	}

	if (mime == QLatin1String(PropertyMimeType::kInteger)) {
		return QString::number(value.toLongLong());
	}

	if (mime == QLatin1String(PropertyMimeType::kNumber)) {
		return QString::number(value.toDouble(), 'g', 15);
	}

	if (mime == QLatin1String(PropertyMimeType::kMultiline)) {
		return QStringLiteral("<<text>>%1<</text>>").arg(textValue(value));
	}

	if (mime == QLatin1String(PropertyMimeType::kJson)) {
		return QStringLiteral("<<json>>%1<</json>>").arg(jsonValue(value));
	}

	if (mime == QLatin1String(PropertyMimeType::kXml) || mime == QLatin1String("text/xml")) {
		return QStringLiteral("<<xml>>%1<</xml>>").arg(textValue(value));
	}

	if (mime == QLatin1String("text/x-ini")) {
		return QStringLiteral("<<ini>>%1<</ini>>").arg(textValue(value));
	}

	if (mime == QLatin1String(PropertyMimeType::kColor)) {
		if (value.metaType().id() == QMetaType::QColor) {
			return quotedValue(value.value<QColor>().name(QColor::HexArgb));
		}
		return quotedValue(value.toString());
	}

	return quotedValue(textValue(value));
}

QString SystemRouterCommandService::lastError() const {
	return d->lastError;
}
