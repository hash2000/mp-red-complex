#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/models/fetch_opt.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class FetchApiService::Private {
public:
	Private(FetchApiService* parent) : q(parent) { }
	FetchApiService* q;

	QNetworkAccessManager* networkManager;
};

FetchApiService::FetchApiService(QObject* parent)
	: d(std::make_unique<Private>(this))
	, QObject(parent) {
	d->networkManager = new QNetworkAccessManager(this);
}

FetchApiService::~FetchApiService() = default;

bool FetchApiService::fetchRequest(
	const FetchApiOpt& request,
	SuccessCallback onSuccess,
	ErrorCallback onError,
	ProgressCallback onProgress) {
	QNetworkReply* reply = nullptr;
	QNetworkRequest options = request.request;

	switch (request.action) {
	case FetchApiActions::Get: {
		reply = d->networkManager->get(options);
		break;
	}
	case FetchApiActions::Post: {
		if (request.body.isNull()) {
			return false;
		}
		reply = d->networkManager->post(options, request.body);
		break;
	}
	case FetchApiActions::Put: {
		reply = d->networkManager->sendCustomRequest(options, "PUT", request.body);
		break;
	}
	case FetchApiActions::Delete: {
		reply = d->networkManager->sendCustomRequest(options, "DELETE", request.body);
		break;
	}
	case FetchApiActions::Patch: {
		reply = d->networkManager->sendCustomRequest(options, "PATCH", request.body);
		break;
	}
	case FetchApiActions::Head: {
		reply = d->networkManager->sendCustomRequest(options, "HEAD", request.body);
		break;
	}
	default:
	return false;
	}

	if (request.sslPolicy == FetchApiSslPolicy::IgnoreErrors) {
		reply->ignoreSslErrors();
	}
	else if (request.sslPolicy == FetchApiSslPolicy::Strict) {
		options.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	}
	else if (request.sslPolicy == FetchApiSslPolicy::TrustCustomCa) {

	}

	connect(reply, &QNetworkReply::finished, this, [reply, onSuccess, onError, onProgress]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			if (onError) {
				onError(reply->error(), reply->errorString(), reply->headers());
			}
			return;
		}

		int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();		
		QByteArray data = reply->readAll();
		if (onSuccess) {
			onSuccess(status, data, reply->headers());
		}
	});

	if (onProgress) {
		connect(reply, &QNetworkReply::downloadProgress, onProgress);
	}

	return true;
}
