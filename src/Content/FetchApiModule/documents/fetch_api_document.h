#pragma once
#include "Content/FetchApiModule/models/fetch_api_result.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <memory>

class FetchApiService;
class FetchApiWidget;
class FormatterPluginManager;
class HighlightingPluginManager;

/// Документ модуля FetchApi: редактор и выполнение REST-запросов.
///
/// Сверху — лента с кнопкой «Отправить»; её параметры (тип запроса и
/// обработка TLS) редактируются в панели «Свойства».
///
/// Документ владеет виджетом-редактором и связывает его с переменными
/// собственного контекста:
/// - при привязке контекста читает параметры запроса (`location`, `method`,
///   `body`, `params`, `headers`, `ssl-policy`, `ssl-ca-file`, `handler`);
/// - перед отправкой записывает параметры редактора обратно в контекст;
/// - результат (`data`, `content-type`, `headers`, `status-code`, ...)
///   сохраняет в контекст, поэтому им могут пользоваться другие документы
///   и команды.
class FetchApiDocument final : public DocumentWidget {
	Q_OBJECT
public:
	FetchApiDocument(FetchApiService* service,
		HighlightingPluginManager* highlighting,
		FormatterPluginManager* formatters,
		QWidget* parent = nullptr);
	~FetchApiDocument() override;

	FetchApiWidget* widget() const;

	void refresh() override;
	bool save() override;
	QWidget* createPropertiesEditor(QWidget* parent) override;
	QList<DocumentPropertyGroup> createPropertyGroups(QWidget* parent) override;

signals:
	/// Запрос завершён; результат уже записан в переменные контекста.
	void requestCompleted(const FetchApiResult& result);

protected:
	void onContextAttached() override;
	void onContextVariableChanged(const QString& name, const QVariant& value) override;

private:
	void applyContextToWidget();
	void writeRequestToContext();
	void onRequestFinished(const FetchApiResult& result);
	void printError(const QString& message);

	struct Private;
	std::unique_ptr<Private> d;
};
