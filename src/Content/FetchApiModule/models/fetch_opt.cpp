#include "Content/FetchApiModule/models/fetch_opt.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QUrl>
#include <QUrlQuery>

namespace {

const QString kContentTypeHeader = QStringLiteral("Content-Type");
const QString kDefaultContentType = QStringLiteral("text/plain; charset=utf-8");
const QString kJsonContentType = QStringLiteral("application/json");

/// Адрес запроса: без схемы подставляется https, иначе строка читается как URL.
QUrl parseLocation(const QString& value) {
	QString location = value.trimmed();
	if (location.isEmpty()) {
		return {};
	}

	if (!location.contains(QStringLiteral("://"))) {
		location.prepend(QStringLiteral("https://"));
	}

	return QUrl::fromUserInput(location);
}

bool looksLikeJson(const QByteArray& body) {
	const auto trimmed = body.trimmed();
	if (!trimmed.startsWith('{') && !trimmed.startsWith('[')) {
		return false;
	}

	QJsonParseError error;
	QJsonDocument::fromJson(trimmed, &error);
	return error.error == QJsonParseError::NoError;
}

} // namespace

class FetchApiOpt::Private {
public:
	QString location;
	FetchApiMethod method = FetchApiMethod::Get;
	QByteArray body;
	FetchApiKeyValues parameters;
	FetchApiKeyValues headers;
	FetchApiSslPolicy sslPolicy = FetchApiSslPolicy::Strict;
	FetchApiHandlerType handler = FetchApiHandlerType::Rest;
	QList<QSslCertificate> customCa;
	int timeoutMs = 0;
};

FetchApiOpt::FetchApiOpt()
	: d(std::make_unique<Private>()) {
}

FetchApiOpt::FetchApiOpt(const QString& location)
	: d(std::make_unique<Private>()) {
	d->location = location;
}

FetchApiOpt::FetchApiOpt(const QString& location, const FetchApiKeyValues& parameters,
	const FetchApiKeyValues& headers)
	: d(std::make_unique<Private>()) {
	d->location = location;
	d->parameters = parameters;
	d->headers = headers;
}

FetchApiOpt::FetchApiOpt(const FetchApiOpt& other)
	: d(std::make_unique<Private>(*other.d)) {
}

FetchApiOpt& FetchApiOpt::operator=(const FetchApiOpt& other) {
	if (this != &other) {
		d = std::make_unique<Private>(*other.d);
	}
	return *this;
}

FetchApiOpt::FetchApiOpt(FetchApiOpt&& other) noexcept = default;

FetchApiOpt& FetchApiOpt::operator=(FetchApiOpt&& other) noexcept = default;

FetchApiOpt::~FetchApiOpt() = default;

bool FetchApiOpt::isValid() const {
	const auto url = parseLocation(d->location);
	return url.isValid() && !url.host().isEmpty();
}

QString FetchApiOpt::describe() const {
	const auto method = QString::fromLatin1(httpMethodName());
	if (d->location.trimmed().isEmpty()) {
		return method;
	}

	return QStringLiteral("%1 %2").arg(method, d->location.trimmed());
}

QString FetchApiOpt::location() const {
	return d->location;
}

void FetchApiOpt::setLocation(const QString& value) {
	d->location = value;
}

FetchApiMethod FetchApiOpt::method() const {
	return d->method;
}

void FetchApiOpt::setMethod(FetchApiMethod value) {
	d->method = value;
}

bool FetchApiOpt::setMethod(const QString& value) {
	const auto parsed = From<FetchApiMethod>::from(value);
	if (!parsed) {
		return false;
	}

	d->method = *parsed;
	return true;
}

QByteArray FetchApiOpt::httpMethodName() const {
	return Format<FetchApiMethod>::format(d->method).toLatin1();
}

const QByteArray& FetchApiOpt::body() const {
	return d->body;
}

void FetchApiOpt::setBody(const QByteArray& value) {
	d->body = value;
}

void FetchApiOpt::setBodyText(const QString& value) {
	d->body = value.toUtf8();
}

const FetchApiKeyValues& FetchApiOpt::parameters() const {
	return d->parameters;
}

void FetchApiOpt::setParameters(const FetchApiKeyValues& value) {
	d->parameters = value;
}

void FetchApiOpt::setParametersFormatted(const QString& value) {
	d->parameters = FetchApiFormat::parseKeyValues(value);
}

void FetchApiOpt::addParameter(const QString& name, const QString& value) {
	if (name.trimmed().isEmpty()) {
		return;
	}

	d->parameters[name.trimmed()] = value;
}

const FetchApiKeyValues& FetchApiOpt::headers() const {
	return d->headers;
}

void FetchApiOpt::setHeaders(const FetchApiKeyValues& value) {
	d->headers = value;
}

void FetchApiOpt::setHeadersFormatted(const QString& value) {
	d->headers = FetchApiFormat::parseKeyValues(value);
}

void FetchApiOpt::addHeader(const QString& name, const QString& value) {
	if (name.trimmed().isEmpty()) {
		return;
	}

	d->headers[name.trimmed()] = value;
}

bool FetchApiOpt::hasHeader(const QString& name) const {
	for (const auto& [header, value] : d->headers) {
		Q_UNUSED(value);
		if (header.compare(name, Qt::CaseInsensitive) == 0) {
			return true;
		}
	}

	return false;
}

FetchApiSslPolicy FetchApiOpt::sslPolicy() const {
	return d->sslPolicy;
}

void FetchApiOpt::setSslPolicy(FetchApiSslPolicy value) {
	d->sslPolicy = value;
}

bool FetchApiOpt::setSslPolicy(const QString& value) {
	const auto parsed = From<FetchApiSslPolicy>::from(value);
	if (!parsed) {
		return false;
	}

	d->sslPolicy = *parsed;
	return true;
}

const QList<QSslCertificate>& FetchApiOpt::customCaCertificates() const {
	return d->customCa;
}

void FetchApiOpt::setCustomCaCertificates(const QList<QSslCertificate>& value) {
	d->customCa = value;
}

bool FetchApiOpt::loadCustomCa(const QByteArray& data, QString* error) {
	if (data.isEmpty()) {
		if (error) {
			*error = QStringLiteral("Пустые данные CA-сертификата");
		}
		return false;
	}

	auto certificates = QSslCertificate::fromData(data, QSsl::Pem);
	if (certificates.isEmpty()) {
		certificates = QSslCertificate::fromData(data, QSsl::Der);
	}

	if (certificates.isEmpty()) {
		if (error) {
			*error = QStringLiteral("CA-сертификат не распознан (ожидается PEM или DER)");
		}
		return false;
	}

	d->customCa = certificates;
	return true;
}

bool FetchApiOpt::loadCustomCaFromFile(const QString& path, QString* error) {
	if (path.trimmed().isEmpty()) {
		if (error) {
			*error = QStringLiteral("Не задан файл CA-сертификата");
		}
		return false;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		if (error) {
			*error = QStringLiteral("Не удалось прочитать файл CA-сертификата «%1»: %2")
				.arg(path, file.errorString());
		}
		return false;
	}

	return loadCustomCa(file.readAll(), error);
}

FetchApiHandlerType FetchApiOpt::handler() const {
	return d->handler;
}

void FetchApiOpt::setHandler(FetchApiHandlerType value) {
	d->handler = value;
}

bool FetchApiOpt::setHandler(const QString& value) {
	const auto parsed = From<FetchApiHandlerType>::from(value);
	if (!parsed) {
		return false;
	}

	d->handler = *parsed;
	return true;
}

int FetchApiOpt::timeoutMs() const {
	return d->timeoutMs;
}

void FetchApiOpt::setTimeoutMs(int value) {
	d->timeoutMs = value;
}

QNetworkRequest FetchApiOpt::request() const {
	const auto url = parseLocation(d->location);
	QUrl requestUrl = url;
	QUrlQuery query(requestUrl);
	for (const auto& [name, value] : d->parameters) {
		query.addQueryItem(name, value);
	}
	requestUrl.setQuery(query);

	QNetworkRequest request(requestUrl);
	for (const auto& [name, value] : d->headers) {
		request.setRawHeader(name.toUtf8(), value.toUtf8());
	}

	// Тело без явного Content-Type отправляется как JSON, если разбирается
	// как JSON, иначе как обычный текст.
	if (!d->body.isEmpty() && !hasHeader(kContentTypeHeader)) {
		request.setHeader(QNetworkRequest::ContentTypeHeader,
			looksLikeJson(d->body) ? kJsonContentType : kDefaultContentType);
	}

	// Политика TrustCustomCa: доверяем только переданным сертификатам.
	if (d->sslPolicy == FetchApiSslPolicy::TrustCustomCa && !d->customCa.isEmpty()) {
		auto configuration = request.sslConfiguration();
		configuration.setCaCertificates(d->customCa);
		request.setSslConfiguration(configuration);
	}

	if (d->timeoutMs > 0) {
		request.setTransferTimeout(d->timeoutMs);
	}

	return request;
}
