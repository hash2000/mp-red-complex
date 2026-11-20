#include "Content/SystemRouter/data_providers/system_router_commands_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_db.h"

#include "Content/DatabaseModule/data_providers/data_blobs_provider_db.h"

class SystemRouterCommandsProviderDb::Private {
public:
	explicit Private(DatabasesService* databasesService)
		: blobs(std::make_unique<DataBlobsProviderDb>(databasesService,
			QString::fromLatin1(SystemRouterDb::kDatabase),
			QString::fromLatin1(SystemRouterDb::kDataBlobsTable))) {
	}

	std::unique_ptr<DataBlobsProviderDb> blobs;

	static SystemRouterCommand toCommand(const DataBlob& blob) {
		SystemRouterCommand command;
		command.id = blob.id;
		command.version = blob.version;
		command.description = blob.description;
		command.script = blob.data;
		return command;
	}

	static DataBlob toBlob(const SystemRouterCommand& command) {
		DataBlob blob;
		blob.id = command.id;
		blob.version = command.version;
		blob.description = command.description;
		blob.data = command.script;
		return blob;
	}
};

SystemRouterCommandsProviderDb::SystemRouterCommandsProviderDb(
	DatabasesService* databasesService)
	: d(std::make_unique<Private>(databasesService)) {
}

SystemRouterCommandsProviderDb::~SystemRouterCommandsProviderDb() = default;

QList<SystemRouterCommand> SystemRouterCommandsProviderDb::commands() const {
	QList<SystemRouterCommand> result;
	for (const auto& blob : d->blobs->blobs()) {
		result.append(Private::toCommand(blob));
	}
	return result;
}

std::optional<SystemRouterCommand> SystemRouterCommandsProviderDb::command(
	const QString& id) const {
	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return std::nullopt;
	}

	const auto blob = d->blobs->blob(normalized);
	if (!blob.has_value()) {
		return std::nullopt;
	}

	return Private::toCommand(*blob);
}

bool SystemRouterCommandsProviderDb::saveCommand(const SystemRouterCommand& command) {
	const auto normalized = normalizeSystemRouterCommandId(command.id);
	if (normalized.isEmpty()) {
		return false;
	}

	auto stored = command;
	stored.id = normalized;
	return d->blobs->saveBlob(Private::toBlob(stored));
}

bool SystemRouterCommandsProviderDb::removeCommand(const QString& id) {
	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return false;
	}

	return d->blobs->removeBlob(normalized);
}

QString SystemRouterCommandsProviderDb::lastError() const {
	return d->blobs->lastError();
}
