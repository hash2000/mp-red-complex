#pragma once
#include "Content/FetchApiModule/models/fetch_api_result.h"
#include "Content/FetchApiModule/models/fetch_api_types.h"

#include <QWidget>

#include <memory>

class FetchApiOpt;
class FetchApiService;
class FormatterPluginManager;
class HighlightingPluginManager;

/// Виджет «аналог Postman»: редактор REST-запроса и просмотр ответа.
///
/// Виджет самостоятелен: он не знает о документе и его контексте. Параметры
/// запроса он отдаёт через `requestOptions()`, выполняет запрос сам (через
/// `FetchApiService`) и сообщает о завершении сигналом `requestFinished`.
/// Документ, которому принадлежит виджет, решает, что делать с результатом.
class FetchApiWidget : public QWidget {
	Q_OBJECT
public:
	FetchApiWidget(FetchApiService* service,
		HighlightingPluginManager* highlighting,
		FormatterPluginManager* formatters,
		QWidget* parent = nullptr);
	~FetchApiWidget() override;

	/// Параметры запроса из полей редактора, включая авторизацию.
	FetchApiOpt requestOptions() const;

	// -------------------------------------------------------- параметры запроса
	QString location() const;
	void setLocation(const QString& location);

	FetchApiMethod method() const;
	void setMethod(FetchApiMethod method);

	QString bodyText() const;
	void setBodyText(const QString& text);

	QString parametersFormatted() const;
	void setParametersFormatted(const QString& parameters);

	QString headersFormatted() const;
	void setHeadersFormatted(const QString& headers);

	FetchApiSslPolicy sslPolicy() const;
	void setSslPolicy(FetchApiSslPolicy policy);

	QString caFile() const;
	void setCaFile(const QString& path);

	FetchApiHandlerType handler() const;
	void setHandler(FetchApiHandlerType handler);

	// ---------------------------------------------------------- состояние ответа
	void showResult(const FetchApiResult& result);
	void showError(const QString& message);
	void showProgress(qint64 received, qint64 total);
	void clearResult();

	bool isBusy() const;

public slots:
	/// Отправить запрос из текущих параметров редактора.
	void sendRequest();

signals:
	void requestStarted();
	void requestFinished(const FetchApiResult& result);
	void requestFailed(const QString& message);

private:
	class Private;
	std::unique_ptr<Private> d;
};
