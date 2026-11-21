#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/models/fetch_opt.h"
#include "Libs/Resources/resources.h"

#include <QElapsedTimer>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QUuid>

namespace {

constexpr int kDefaultTimeoutMs = 30000;
const char* kTimeoutVariable = "fetch-api.timeoutMs";
const char* kUserAgentVariable = "fetch-api.userAgent";
const char* kUserAgentDefault = "RedComplex-FetchApi/1.0";
const char* kUserAgentHeader = "User-Agent";

} // namespace

class FetchApiService::Private {
public:
	/// Состояние одного выполняющегося запроса.
	struct Pending {
		QString id;
		QElapsedTimer timer;
		FetchApiMethod method = FetchApiMethod::Get;
		QString location;
		FetchApiSslPolicy sslPolicy = FetchApiSslPolicy::Strict;
		QList<QSslCertificate> customCa;
	};

	Private(FetchApiService* parent) : q(parent) { }
	FetchApiService* q;

	Resources* resources = nullptr;
	QNetworkAccessManager* networkManager = nullptr;
	QHash<QNetworkReply*, Pending> pending;
	QString lastError;

	bool fail(const QString& message);
	int defaultTimeoutMs() const;
	QString userAgent() const;

	void applySslPolicy(QNetworkReply* reply, const Pending& state);
	FetchApiResult buildResult(QNetworkReply* reply, const Pending& state) const;
	static QString transportErrorMessage(QNetworkReply* reply);
};

FetchApiService::FetchApiService(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	// Менеджер — дочерний объект сервиса: владелец сервиса — реестр сервисов,
	// а сетевые объекты живут ровно столько же, сколько сам сервис.
	d->networkManager = new QNetworkAccessManager(this);

	qRegisterMetaType<FetchApiResult>("FetchApiResult");
}

FetchApiService::~FetchApiService() = default;

bool FetchApiService::Private::fail(const QString& message) {
	lastError = message;
	emit q->errorOccurred(message);
	return false;
}

int FetchApiService::Private::defaultTimeoutMs() const {
	int value = kDefaultTimeoutMs;
	if (resources) {
		value = resources->Variables.get(QString::fromLatin1(kTimeoutVariable), value).toInt();
	}

	return value > 0 ? value : kDefaultTimeoutMs;
}

QString FetchApiService::Private::userAgent() const {
	const auto fallback = QString::fromLatin1(kUserAgentDefault);
	if (!resources) {
		return fallback;
	}

	const auto value = resources->Variables
		.get(QString::fromLatin1(kUserAgentVariable), fallback).toString();
	return value.isEmpty() ? fallback : value;
}

void FetchApiService::Private::applySslPolicy(QNetworkReply* reply, const Pending& state) {
	if (!reply) {
		return;
	}

	// Политика проверяется на каждую ошибку TLS: Strict — ошибку не подавляем,
	// запрос завершится со штатной ошибкой проверки сертификата.
	QObject::connect(reply, &QNetworkReply::sslErrors, q,
		[reply, state](const QList<QSslError>& errors) {
			switch (state.sslPolicy) {
			case FetchApiSslPolicy::IgnoreErrors:
				reply->ignoreSslErrors();
				break;
			case FetchApiSslPolicy::TrustCustomCa: {
				// Доверяем только сертификатам из набора: остальные ошибки
				// остаются и прерывают запрос.
				QList<QSslError> trusted;
				for (const auto& error : errors) {
					const auto certificate = error.certificate();
					if (certificate.isNull()) {
						continue;
					}

					for (const auto& ca : state.customCa) {
						if (ca == certificate) {
							trusted.append(error);
							break;
						}
					}
				}

				if (!trusted.isEmpty()) {
					reply->ignoreSslErrors(trusted);
				}
				break;
			}
			case FetchApiSslPolicy::Strict:
				break;
			}
		});
}

QString FetchApiService::Private::transportErrorMessage(QNetworkReply* reply) {
	if (!reply) {
		return QStringLiteral("Запрос не выполнен");
	}

	switch (reply->error()) {
	case QNetworkReply::TimeoutError:
	case QNetworkReply::OperationCanceledError:
		return QStringLiteral("Превышено время ожидания ответа");
	case QNetworkReply::HostNotFoundError:
		return QStringLiteral("Узел не найден: %1").arg(reply->url().host());
	case QNetworkReply::ConnectionRefusedError:
		return QStringLiteral("Соединение отклонено: %1").arg(reply->url().host());
	case QNetworkReply::SslHandshakeFailedError:
		return QStringLiteral("Ошибка TLS: %1").arg(reply->errorString());
	default:
		return reply->errorString();
	}
}

FetchApiResult FetchApiService::Private::buildResult(QNetworkReply* reply,
	const Pending& state) const {
	FetchApiResult result;
	result.requestId = state.id;
	result.method = state.method;
	result.location = state.location;
	result.elapsedMs = state.timer.isValid() ? state.timer.elapsed() : 0;

	if (!reply) {
		result.failed = true;
		result.errorString = QStringLiteral("Запрос не выполнен");
		return result;
	}

	result.url = reply->url();
	result.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	result.statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
	result.headers = reply->headers();
	result.data = reply->readAll();
	result.size = result.data.size();
	result.errorCode = static_cast<int>(reply->error());

	if (reply->error() != QNetworkReply::NoError) {
		result.failed = true;
		result.errorString = transportErrorMessage(reply);
		return result;
	}

	// Транспорт ошибки не дал: запрос выполнен, дальше оценивается HTTP-ответ.
	result.failed = false;

	if (result.statusCode == 0) {
		result.failed = true;
		result.errorString = QStringLiteral("Ответ не содержит HTTP-статуса");
		return result;
	}

	// Транспорт ошибки не дал, но HTTP-ответ сообщает об ошибке: тело и
	// заголовки сохраняются, запрос считается неуспешным.
	if (result.statusCode >= 400) {
		result.failed = true;
		result.errorString = result.statusText.isEmpty()
			? QStringLiteral("HTTP %1").arg(result.statusCode)
			: QStringLiteral("HTTP %1 %2").arg(result.statusCode).arg(result.statusText);
	}

	return result;
}

bool FetchApiService::execute(const FetchApiOpt& request, CompletionHandler onFinished,
	ProgressHandler onProgress) {
	d->lastError.clear();

	if (request.handler() != FetchApiHandlerType::Rest) {
		return d->fail(tr("Обработчик «%1» не поддерживается")
			.arg(Format<FetchApiHandlerType>::format(request.handler())));
	}

	if (request.location().trimmed().isEmpty()) {
		return d->fail(tr("Не задан адрес запроса"));
	}

	if (!request.isValid()) {
		return d->fail(tr("Некорректный адрес запроса: %1").arg(request.location()));
	}

	if (request.sslPolicy() == FetchApiSslPolicy::TrustCustomCa
		&& request.customCaCertificates().isEmpty()) {
		return d->fail(tr("Политика TLS «trust-custom-ca» требует CA-сертификат"));
	}

	if (!d->networkManager) {
		return d->fail(tr("Сетевое окружение недоступно"));
	}

	auto networkRequest = request.request();
	if (networkRequest.transferTimeout() <= 0) {
		networkRequest.setTransferTimeout(d->defaultTimeoutMs());
	}

	if (!networkRequest.hasRawHeader(kUserAgentHeader)) {
		networkRequest.setRawHeader(kUserAgentHeader, d->userAgent().toUtf8());
	}

	QNetworkReply* reply = nullptr;
	switch (request.method()) {
	case FetchApiMethod::Get:
		reply = d->networkManager->get(networkRequest, request.body());
		break;
	case FetchApiMethod::Post:
		reply = d->networkManager->post(networkRequest, request.body());
		break;
	case FetchApiMethod::Put:
		reply = d->networkManager->sendCustomRequest(networkRequest, "PUT", request.body());
		break;
	case FetchApiMethod::Delete:
		reply = d->networkManager->sendCustomRequest(networkRequest, "DELETE", request.body());
		break;
	case FetchApiMethod::Patch:
		reply = d->networkManager->sendCustomRequest(networkRequest, "PATCH", request.body());
		break;
	case FetchApiMethod::Head:
		reply = d->networkManager->head(networkRequest);
		break;
	}

	if (!reply) {
		return d->fail(tr("Не удалось отправить запрос: %1").arg(request.describe()));
	}

	Private::Pending state;
	state.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	state.timer.start();
	state.method = request.method();
	state.location = request.location();
	state.sslPolicy = request.sslPolicy();
	state.customCa = request.customCaCertificates();

	d->pending.insert(reply, state);
	d->applySslPolicy(reply, state);

	const auto requestId = state.id;
	connect(reply, &QNetworkReply::downloadProgress, this,
		[this, requestId](qint64 received, qint64 total) {
			emit downloadProgress(requestId, received, total);
		});

	if (onProgress) {
		connect(reply, &QNetworkReply::downloadProgress, this,
			[onProgress](qint64 received, qint64 total) {
				onProgress(received, total);
			});
	}

	connect(reply, &QNetworkReply::finished, this, [this, reply, onFinished]() {
		const auto state = d->pending.take(reply);
		const auto result = d->buildResult(reply, state);
		reply->deleteLater();

		emit requestFinished(result);
		if (result.failed) {
			emit requestFailed(result.requestId, result.errorMessage());
		}

		if (onFinished) {
			onFinished(result);
		}
	});

	emit requestStarted(requestId);
	return true;
}

QString FetchApiService::lastError() const {
	return d->lastError;
}

int FetchApiService::pendingRequests() const {
	return static_cast<int>(d->pending.size());
}

void FetchApiService::cancelAll() {
	const auto replies = d->pending.keys();
	for (auto* reply : replies) {
		if (reply) {
			reply->abort();
		}
	}
}
