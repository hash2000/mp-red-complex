#pragma once
#include "Libs/Base/format.h"
#include "Libs/Base/from.h"

#include <QByteArray>
#include <QHttpHeaders>
#include <QString>
#include <QStringList>

#include <map>
#include <optional>

/// HTTP-метод REST-запроса. Значение по умолчанию — GET: у модели не может
/// быть «неинициализированного» метода.
enum class FetchApiMethod {
	Get,
	Post,
	Put,
	Delete,
	Patch,
	Head,
};

template <> struct Format<FetchApiMethod> {
	static QString format(const FetchApiMethod& value) {
		switch (value) {
		case FetchApiMethod::Get: return QStringLiteral("GET");
		case FetchApiMethod::Post: return QStringLiteral("POST");
		case FetchApiMethod::Put: return QStringLiteral("PUT");
		case FetchApiMethod::Delete: return QStringLiteral("DELETE");
		case FetchApiMethod::Patch: return QStringLiteral("PATCH");
		case FetchApiMethod::Head: return QStringLiteral("HEAD");
		}
		return QStringLiteral("GET");
	}
};

template <> struct From<FetchApiMethod> {
	static std::optional<FetchApiMethod> from(const QString& value) {
		const auto normalized = value.trimmed().toLower();
		if (normalized == "get") return FetchApiMethod::Get;
		if (normalized == "post") return FetchApiMethod::Post;
		if (normalized == "put") return FetchApiMethod::Put;
		if (normalized == "delete") return FetchApiMethod::Delete;
		if (normalized == "patch") return FetchApiMethod::Patch;
		if (normalized == "head") return FetchApiMethod::Head;
		return std::nullopt;
	}
};

/// Тип обработчика запроса. Сейчас реализован только REST; остальные виды
/// (gRPC и т. п.) добавляются новым значением и ветками в сервисе.
enum class FetchApiHandlerType {
	Rest,
};

template <> struct Format<FetchApiHandlerType> {
	static QString format(const FetchApiHandlerType& value) {
		switch (value) {
		case FetchApiHandlerType::Rest: return QStringLiteral("rest");
		}
		return QStringLiteral("rest");
	}
};

template <> struct From<FetchApiHandlerType> {
	static std::optional<FetchApiHandlerType> from(const QString& value) {
		const auto normalized = value.trimmed().toLower();
		if (normalized.isEmpty() || normalized == "rest" || normalized == "http") {
			return FetchApiHandlerType::Rest;
		}
		return std::nullopt;
	}
};

/// Политика обработки ошибок TLS.
/// Strict — любой сбой проверки сертификата прерывает запрос;
/// TrustCustomCa — доверяем только перечисленным сертификатам (CA);
/// IgnoreErrors — проверка ошибок TLS отключена.
enum class FetchApiSslPolicy {
	Strict,
	TrustCustomCa,
	IgnoreErrors,
};

template <> struct Format<FetchApiSslPolicy> {
	static QString format(const FetchApiSslPolicy& value) {
		switch (value) {
		case FetchApiSslPolicy::Strict: return QStringLiteral("strict");
		case FetchApiSslPolicy::TrustCustomCa: return QStringLiteral("trust-custom-ca");
		case FetchApiSslPolicy::IgnoreErrors: return QStringLiteral("ignore-errors");
		}
		return QStringLiteral("strict");
	}
};

template <> struct From<FetchApiSslPolicy> {
	static std::optional<FetchApiSslPolicy> from(const QString& value) {
		QString normalized = value.trimmed().toLower();
		normalized.replace('_', '-');

		if (normalized.isEmpty() || normalized == "strict") {
			return FetchApiSslPolicy::Strict;
		}
		if (normalized == "trust-custom-ca" || normalized == "trustcustomca"
			|| normalized == "custom-ca" || normalized == "trust") {
			return FetchApiSslPolicy::TrustCustomCa;
		}
		if (normalized == "ignore-errors" || normalized == "ignore"
			|| normalized == "insecure") {
			return FetchApiSslPolicy::IgnoreErrors;
		}
		return std::nullopt;
	}
};

/// Пары «имя → значение»: заголовки запроса и query-параметры.
using FetchApiKeyValues = std::map<QString, QString>;

/// Имена переменных контекста и событие модуля. Документ, команда и
/// результат пишут/читают только эти имена.
namespace FetchApiVariables {
	/// Входные параметры запроса.
	inline constexpr const char* kLocation = "location";
	inline constexpr const char* kUrlAlias = "url";
	inline constexpr const char* kMethod = "method";
	inline constexpr const char* kBody = "body";
	inline constexpr const char* kParams = "params";
	inline constexpr const char* kParamsAlias = "query";
	inline constexpr const char* kHeaders = "headers";
	inline constexpr const char* kSslPolicy = "ssl-policy";
	inline constexpr const char* kSslCaFile = "ssl-ca-file";
	inline constexpr const char* kHandler = "handler";

	/// Результат запроса.
	inline constexpr const char* kData = "data";
	inline constexpr const char* kContentType = "content-type";
	inline constexpr const char* kContentTypeAlias = "contentType";
	inline constexpr const char* kStatusCode = "status-code";
	inline constexpr const char* kStatusText = "status-text";
	inline constexpr const char* kError = "error";
	inline constexpr const char* kErrorCode = "error-code";
	inline constexpr const char* kElapsedMs = "elapsed-ms";
	inline constexpr const char* kSize = "size";
	/// Фактический адрес ответа (после редиректов); совпадает с входным
	/// псевдонимом `url`, поэтому после запроса в переменной лежит ответ.
	inline constexpr const char* kUrl = "url";

	/// Событие контекста: запрос завершён, результат уже записан в переменные.
	inline constexpr const char* kCompleteEvent = "fetch-api.complete";
} // namespace FetchApiVariables

/// Разбор и форматирование пар «имя → значение».
///
/// Понимает три формы записи:
/// - по строке на пару (`Content-Type: application/json`);
/// - через `;` (`a=1; b=2`), как в командах;
/// - как query-строку URL (`?a=1&b=2`).
/// Разделитель пары — первый `:` или `=`. Пустые строки и строки,
/// начинающиеся с `#`, пропускаются.
namespace FetchApiFormat {
	FetchApiKeyValues parseKeyValues(const QString& text);
	QString formatKeyValues(const FetchApiKeyValues& values);

	/// Заголовки ответа в формате пар «имя → значение».
	FetchApiKeyValues headersMap(const QHttpHeaders& headers);
	/// Заголовки ответа по строке на заголовок (`Имя: значение`).
	QString formatHeaders(const QHttpHeaders& headers);
	/// Имя заголовка в привычном виде (`Content-Type` вместо `content-type`).
	QString headerName(const QString& name);
	/// Значение заголовка без учёта регистра имени; пустая строка, если нет.
	QString headerValue(const QHttpHeaders& headers, const QString& name);
	/// Все значения заголовка (например, несколько Set-Cookie).
	QStringList headerValues(const QHttpHeaders& headers, const QString& name);
} // namespace FetchApiFormat
