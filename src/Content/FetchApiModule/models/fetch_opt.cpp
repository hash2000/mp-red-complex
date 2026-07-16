#include "Content/FetchApiModule/models/fetch_opt.h"

#include <QNetworkRequest>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>

class FetchApiOpt::Private {
public:
	Private(FetchApiOpt* parent) : q(parent) { }
	FetchApiOpt* q;

	std::shared_ptr<QNetworkRequest> request;
	FetchApiMethod method;
	QByteArray body;
	FetchApiSslPolicy sslPolicy = FetchApiSslPolicy::Strict;

	void createRequest(const QString& location, const std::map<QString, QString>& parameters);

	void clearHeaders();

	// прочитать из отформатированной строки
	// {name}:{value};{name}:{value}; .. 
	void appendHeadersFormatted(const QString& headers);
	void appendHeaders(const std::map<QString, QString>& headers);
	void appendHeader(QAnyStringView name, QAnyStringView value);

};


FetchApiOpt::FetchApiOpt(
	const QString& location,
	const std::map<QString, QString>& parameters,
	const std::map<QString, QString>& headers)
	: d(std::make_unique<Private>(this)) {
	d->createRequest(location, parameters);
	d->appendHeaders(headers);
}

FetchApiOpt::FetchApiOpt(
	const QString& location,
	const std::map<QString, QString>& parameters,
	const QString& headersFormatted)
	: d(std::make_unique<Private>(this)) {
	d->createRequest(location, parameters);
	d->appendHeadersFormatted(headersFormatted);
}

FetchApiOpt::~FetchApiOpt() = default;

void FetchApiOpt::Private::createRequest(const QString& location, const std::map<QString, QString>& parameters) {
	auto parseWebUrl = [](const QString& input) {
		QString str = input.trimmed();
		if (!str.contains("://")) {
			str.prepend("https://");
		}
		return QUrl::fromUserInput(str);
	};

	QUrl url = parseWebUrl(location);
	QUrlQuery query(url);

	for (const auto& [key, value] : parameters) {
		query.addQueryItem(key, value);
	}

	url.setQuery(query);

	request = std::make_shared<QNetworkRequest>(QString::fromUtf8(url.toEncoded()));
}

std::shared_ptr<QNetworkRequest> FetchApiOpt::request() const {
	return d->request;
}

bool FetchApiOpt::setMethod(FetchApiMethod value) {
	d->method = value;
	return true;
}

bool FetchApiOpt::setMethod(const QString& value) {
	auto optMethord = From<FetchApiMethod>::from(value);
	if (!optMethord) {
		return false;
	}

	return setMethod(optMethord.value());
}

FetchApiMethod FetchApiOpt::method() const {
	return d->method;
}

void FetchApiOpt::setBody(const QByteArray& value) {
	const auto reqHeaders = d->request->headers();
	const auto contentType = reqHeaders.value(QHttpHeaders::WellKnownHeader::ContentType);
	if (!contentType.isEmpty() && contentType.contains("application/json")) {
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(value, &parseError);
		if (parseError.error == QJsonParseError::NoError) {
			d->body = doc.toJson(QJsonDocument::Compact);
			return;
		}
	}

	d->body = value;
}

const QByteArray FetchApiOpt::body() const {
	return d->body;
}

void FetchApiOpt::setSslPolicy(FetchApiSslPolicy value) {
	d->sslPolicy = value;
}

FetchApiSslPolicy FetchApiOpt::sslPolicy() const {
	return d->sslPolicy;
}

void FetchApiOpt::Private::clearHeaders() {
	request->headers().clear();
}

void FetchApiOpt::Private::appendHeader(QAnyStringView name, QAnyStringView value) {
	request->setRawHeader(name.toString().toUtf8(), value.toString().toUtf8());
}

void FetchApiOpt::Private::appendHeaders(const std::map<QString, QString>& headers) {
	for (const auto& [key, value] : headers) {
		appendHeader(key, value);
	}
}

void FetchApiOpt::Private::appendHeadersFormatted(const QString& headers) {
	const auto headersParsed = headers.split(";", Qt::SkipEmptyParts);
	for (const auto hdr : headersParsed) {
		const auto kv = hdr.split(":", Qt::SkipEmptyParts);
		if (kv.isEmpty() || kv.count() != 2) {
			continue;
		}

		request->setRawHeader(kv[0].toUtf8(), kv[1].toUtf8());
	}
}
