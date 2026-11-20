#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Content/SystemRouter/models/system_router_command.h"

#include <QUuid>

class SystemRouterBlobService::Private {
public:
	explicit Private(SystemRouterBlobService* parent)
		: q(parent) {
	}

	SystemRouterBlobService* q;
	IDataBlobsProvider* provider = nullptr;
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

SystemRouterBlobService::SystemRouterBlobService(IDataBlobsProvider* provider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->provider = provider;
}

SystemRouterBlobService::~SystemRouterBlobService() = default;

IDataBlobsProvider* SystemRouterBlobService::dataProvider() const {
	return d->provider;
}

QList<DataBlob> SystemRouterBlobService::blobs() const {
	d->clearError();

	if (!d->provider) {
		d->lastError = tr("Поставщик блоков данных не задан");
		return {};
	}

	auto result = d->provider->blobs();
	d->lastError = d->provider->lastError();
	return result;
}

std::optional<DataBlob> SystemRouterBlobService::blob(const QString& id) const {
	if (!d->provider) {
		d->lastError = tr("Поставщик блоков данных не задан");
		return std::nullopt;
	}

	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return std::nullopt;
	}

	auto result = d->provider->blob(normalized);
	d->lastError = d->provider->lastError();
	return result;
}

QString SystemRouterBlobService::data(const QString& id) const {
	const auto value = blob(id);
	return value.has_value() ? value->data : QString();
}

bool SystemRouterBlobService::saveData(const QString& id, const QString& data) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик блоков данных не задан"));
	}

	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty() || QUuid::fromString(normalized).isNull()) {
		return d->fail(tr("Идентификатор блока должен быть GUID"));
	}

	DataBlob blob;
	blob.id = normalized;
	blob.data = data;
	if (const auto existing = d->provider->blob(normalized); existing.has_value()) {
		blob.version = existing->version;
		blob.description = existing->description;
	}

	if (!d->provider->saveBlob(blob)) {
		return d->fail(d->provider->lastError());
	}

	return true;
}

QString SystemRouterBlobService::createData(const QString& data) {
	d->clearError();

	if (!d->provider) {
		d->fail(tr("Поставщик блоков данных не задан"));
		return {};
	}

	DataBlob blob;
	blob.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	blob.data = data;

	if (!d->provider->saveBlob(blob)) {
		d->fail(d->provider->lastError());
		return {};
	}

	return blob.id;
}

bool SystemRouterBlobService::removeBlob(const QString& id) {
	d->clearError();

	if (!d->provider) {
		return d->fail(tr("Поставщик блоков данных не задан"));
	}

	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return d->fail(tr("Идентификатор блока не задан"));
	}

	if (!d->provider->removeBlob(normalized)) {
		return d->fail(d->provider->lastError());
	}

	return true;
}

QString SystemRouterBlobService::lastError() const {
	return d->lastError;
}
