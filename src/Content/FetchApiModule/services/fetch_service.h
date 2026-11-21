#pragma once
#include "Content/FetchApiModule/models/fetch_api_result.h"

#include <QObject>

#include <functional>
#include <memory>

class FetchApiOpt;
class Resources;

/// Сервис выполнения HTTP-запросов (REST).
///
/// Сервис не знает о контекстах и документах: он получает готовые параметры
/// запроса (`FetchApiOpt`) и отдаёт `FetchApiResult` — и для успеха, и для
/// ошибки. Вызывающая сторона сама решает, что делать с результатом:
/// документ и команда записывают его в переменные контекста.
class FetchApiService : public QObject {
	Q_OBJECT
public:
	using CompletionHandler = std::function<void(const FetchApiResult& result)>;
	using ProgressHandler = std::function<void(qint64 received, qint64 total)>;

	/// @param resources ресурсы приложения: значения по умолчанию
	///        (`fetch-api.timeoutMs`, `fetch-api.userAgent`). Может быть nullptr.
	explicit FetchApiService(Resources* resources, QObject* parent = nullptr);
	~FetchApiService() override;

	/// Отправить запрос. Завершение приходит асинхронно — в `onFinished`.
	/// false — запрос не отправлен, причина в lastError().
	bool execute(const FetchApiOpt& request,
		CompletionHandler onFinished,
		ProgressHandler onProgress = nullptr);

	/// Причина последней синхронной ошибки (незаполненный адрес, политика TLS,
	/// неизвестный обработчик).
	QString lastError() const;

	/// Сколько запросов сейчас выполняется.
	int pendingRequests() const;

	/// Прервать все запросы; завершение придёт с ошибкой отмены.
	void cancelAll();

signals:
	void requestStarted(const QString& requestId);
	void requestFinished(const FetchApiResult& result);
	void requestFailed(const QString& requestId, const QString& message);
	void errorOccurred(const QString& message);
	void downloadProgress(const QString& requestId, qint64 received, qint64 total);

private:
	class Private;
	std::unique_ptr<Private> d;
};
