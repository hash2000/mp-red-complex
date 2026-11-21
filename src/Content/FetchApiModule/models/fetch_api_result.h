#pragma once
#include "Content/FetchApiModule/models/fetch_api_types.h"

#include <QByteArray>
#include <QHttpHeaders>
#include <QList>
#include <QMetaType>
#include <QPair>
#include <QString>
#include <QUrl>

class CommandContext;

/// Результат выполнения REST-запроса.
///
/// Заполняется сервисом один раз — и для успешного ответа, и для ошибки, —
/// поэтому у вызывающей стороны нет ветвления на «успех/ошибку»: смотрите
/// `failed` и `errorMessage()`. `applyToContext()` переносит результат
/// в переменные контекста по общим правилам модуля.
struct FetchApiResult {
	QString requestId;                 ///< идентификатор запроса (UUID)
	FetchApiMethod method = FetchApiMethod::Get;
	QString location;                  ///< адрес из запроса
	QUrl url;                          ///< фактический адрес ответа (с редиректами)

	int statusCode = 0;                ///< код HTTP; 0 — ответа нет
	QString statusText;                ///< причина из ответа («OK», «Not Found»)
	QByteArray data;                   ///< тело ответа
	QHttpHeaders headers;              ///< заголовки ответа

	bool failed = true;                ///< сеть/HTTP/политика TLS: запрос не удался
	int errorCode = 0;                 ///< QNetworkReply::NetworkError либо 0
	QString errorString;               ///< текст ошибки для пользователя
	qint64 elapsedMs = 0;              ///< время выполнения, мс
	qint64 size = 0;                   ///< размер тела, байт

	/// 2xx и нет ошибки транспорта.
	bool isSuccess() const;
	/// true — ответ получен, но код HTTP >= 400.
	bool isHttpError() const;

	QString text() const;
	QString contentType() const;
	QString headersFormatted() const;
	/// Куки из заголовков `Set-Cookie`: имя и атрибуты (без значения).
	QList<QPair<QString, QString>> cookies() const;

	/// Текст ошибки для консоли и панели свойств; пустая строка у успеха.
	QString errorMessage() const;

	/// Записать результат в переменные контекста и отправить событие
	/// `fetch-api.complete`.
	void applyToContext(CommandContext& context) const;

	/// Имя события контекста, которым завершается запрос.
	static const QString& completeEventName();
};

Q_DECLARE_METATYPE(FetchApiResult)
