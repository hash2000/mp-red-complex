#pragma once
#include "Content/FetchApiModule/models/fetch_api_types.h"

#include <QByteArray>
#include <QList>
#include <QSslCertificate>
#include <QString>

#include <memory>

class QNetworkRequest;

/// Параметры одного REST-запроса.
///
/// Модель хранит всё, что описывает запрос: адрес, метод, тело, query-параметры,
/// заголовки, политику TLS и тип обработчика. `request()` собирает из этих
/// данных `QNetworkRequest`; повторный вызов возвращает новое представление,
/// поэтому изменение параметров не требует пересоздания модели.
class FetchApiOpt {
public:
	FetchApiOpt();
	explicit FetchApiOpt(const QString& location);
	FetchApiOpt(const QString& location, const FetchApiKeyValues& parameters,
		const FetchApiKeyValues& headers = FetchApiKeyValues());

	FetchApiOpt(const FetchApiOpt& other);
	FetchApiOpt& operator=(const FetchApiOpt& other);
	FetchApiOpt(FetchApiOpt&& other) noexcept;
	FetchApiOpt& operator=(FetchApiOpt&& other) noexcept;
	~FetchApiOpt();

	/// Есть ли минимально необходимое для запроса: непустой адрес и корректный URL.
	bool isValid() const;
	/// Короткое описание запроса для вывода: «GET https://host/path».
	QString describe() const;

	QString location() const;
	void setLocation(const QString& value);

	FetchApiMethod method() const;
	void setMethod(FetchApiMethod value);
	/// Разбор метода из строки без учёта регистра; false — метод неизвестен.
	bool setMethod(const QString& value);
	/// Имя метода для HTTP: `GET`, `POST`, ...
	QByteArray httpMethodName() const;

	const QByteArray& body() const;
	void setBody(const QByteArray& value);
	void setBodyText(const QString& value);

	const FetchApiKeyValues& parameters() const;
	void setParameters(const FetchApiKeyValues& value);
	void setParametersFormatted(const QString& value);
	void addParameter(const QString& name, const QString& value);

	const FetchApiKeyValues& headers() const;
	void setHeaders(const FetchApiKeyValues& value);
	void setHeadersFormatted(const QString& value);
	void addHeader(const QString& name, const QString& value);
	/// true — заголовок задан (имя сравнивается без учёта регистра).
	bool hasHeader(const QString& name) const;

	FetchApiSslPolicy sslPolicy() const;
	void setSslPolicy(FetchApiSslPolicy value);
	bool setSslPolicy(const QString& value);

	const QList<QSslCertificate>& customCaCertificates() const;
	void setCustomCaCertificates(const QList<QSslCertificate>& value);
	/// Загрузить сертификаты из PEM/DER-данных; false — сертификаты не найдены.
	bool loadCustomCa(const QByteArray& data, QString* error = nullptr);
	/// Загрузить сертификаты из файла (PEM или DER).
	bool loadCustomCaFromFile(const QString& path, QString* error = nullptr);

	FetchApiHandlerType handler() const;
	void setHandler(FetchApiHandlerType value);
	bool setHandler(const QString& value);

	/// Тайм-аут запроса в миллисекундах; 0 — значение по умолчанию сервиса.
	int timeoutMs() const;
	void setTimeoutMs(int value);

	/// Собрать запрос Qt из параметров модели.
	QNetworkRequest request() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
