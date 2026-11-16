#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
#include "Content/FetchApiModule/services/fetch_service.h"

#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/models/fetch_opt.h"

#include "Libs/Engine/services/services_registry.h"
#include "Libs/Base/extensions/text_edit_extensions.h"
#include "Libs/BaseWidgets/key_value/key_value_editor_widget.h"

#include "Content/CodeEditorWidget/widgets/code_editor_widget.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QFont>
#include <QGroupBox>
#include <QProgressBar>
#include <QElapsedTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QStyle>

namespace {
/// Переприменяет стиль темы после изменения динамического свойства.
void refreshStyle(QWidget* widget) {
	if (!widget) {
		return;
	}

	widget->style()->unpolish(widget);
	widget->style()->polish(widget);
	widget->update();
}
}

class FetchApiWidget::Private {
public:
	Private(FetchApiWidget* parent) : q(parent) {}
	FetchApiWidget* q;

	std::shared_ptr<CommandContext> commandContext;
	FetchApiService* fetchApiService = nullptr;
	QLineEdit* requestUrlEdit = nullptr;
	QComboBox* requestMethodCombo = nullptr;
	QPlainTextEdit* requestBodyEdit = nullptr;
	KeyValueEditorWidget* requestHeaders = nullptr;
	KeyValueEditorWidget* requestParams = nullptr;
	QWidget* requestAuth = nullptr;
	QLabel* statusLabel = nullptr;
	QLabel* timeLabel = nullptr;
	QLabel* sizeLabel = nullptr;
	QProgressBar* requestProgressBar;
	QElapsedTimer requestTimer;

	CodeEditorWidget* responseBodyEdit = nullptr;
	KeyValueEditorWidget* responseHeaders = nullptr;
	KeyValueEditorWidget* responseCoocies = nullptr;

	void setupUI();
	void setStatus(int statusCode, const QString& message);
	QString formatBytes(qint64 bytes) const;
	void setResponseHeaders(const QHttpHeaders& headers);

	void fetchSuccess(int statusCode, const QByteArray& data, const QHttpHeaders& headers);
	void fetchError(int errorCode, const QString& errorString, const QHttpHeaders& headers);
	void fetchProgress(qint64 received, qint64 total);
};

FetchApiWidget::FetchApiWidget(std::shared_ptr<CommandContext> commandContext, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {
	d->commandContext = commandContext;
	d->fetchApiService = commandContext->commandController()->services()->get<FetchApiService>(),
	d->setupUI();
}

FetchApiWidget::~FetchApiWidget() = default;

void FetchApiWidget::Private::setupUI() {
	// Главный вертикальный layout
	QVBoxLayout* mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(15, 15, 15, 15);
	mainLayout->setSpacing(10);

	// Верхняя панель (Метод, URL, Кнопка Send)
	QHBoxLayout* topBarLayout = new QHBoxLayout();

	// Выпадающий список методов
	requestMethodCombo = new QComboBox();
	requestMethodCombo->setObjectName(QStringLiteral("FetchMethodCombo"));
	requestMethodCombo->addItems({ "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD" });
	requestMethodCombo->setMinimumWidth(110);

	// Поле ввода URL
	requestUrlEdit = new QLineEdit();
	requestUrlEdit->setObjectName(QStringLiteral("FetchUrlEdit"));
	requestUrlEdit->setPlaceholderText("Введите URL запроса (например, https://api.example.com/v1/users)");

	// Кнопка Send 
	QPushButton* sendBtn = new QPushButton("Send");
	sendBtn->setObjectName(QStringLiteral("FetchSendButton"));
	sendBtn->setMinimumWidth(90);
	sendBtn->setCursor(Qt::PointingHandCursor);

	topBarLayout->addWidget(requestMethodCombo);
	topBarLayout->addWidget(requestUrlEdit, 1);
	topBarLayout->addWidget(sendBtn);

	connect(sendBtn, &QPushButton::clicked, q, &FetchApiWidget::onSendClicked);

	// Разделитель (Request и Response)
	QSplitter* splitter = new QSplitter(Qt::Vertical);

	// Область запроса (request)
	QWidget* requestWidget = new QWidget();
	QVBoxLayout* reqLayout = new QVBoxLayout(requestWidget);
	reqLayout->setContentsMargins(0, 0, 0, 0);

	QTabWidget* reqTabs = new QTabWidget();

	// Вкладка Body
	requestBodyEdit = new QPlainTextEdit();
	requestBodyEdit->setPlaceholderText("{\n  \"key\": \"value\"\n}");
	QFont monoFont("Consolas", 11); // Моноширинный шрифт для кода
	if (!monoFont.exactMatch()) {
		monoFont = QFont("Courier New", 11);
	}
	requestBodyEdit->setObjectName(QStringLiteral("FetchBodyEdit"));
	requestBodyEdit->setFont(monoFont);
	Extensions::TextEdit::setTabDistance(requestBodyEdit, 2);


	requestHeaders = new KeyValueEditorWidget();
	requestParams = new KeyValueEditorWidget();
	requestAuth = new QWidget();

	reqTabs->addTab(requestBodyEdit, "Body");
	reqTabs->addTab(requestHeaders, "Headers");
	reqTabs->addTab(requestParams, "Params");
	reqTabs->addTab(requestAuth, "Auth");

	reqLayout->addWidget(reqTabs);

	// Область ответа (response)
	QWidget* responseWidget = new QWidget();
	QVBoxLayout* respLayout = new QVBoxLayout(responseWidget);
	respLayout->setContentsMargins(0, 0, 0, 0);

	// Строка статуса ответа (как в Postman)
	QHBoxLayout* statusLayout = new QHBoxLayout();
	statusLabel = new QLabel("");
	statusLabel->setObjectName(QStringLiteral("FetchStatus"));

	timeLabel = new QLabel("Time: ... ms");
	sizeLabel = new QLabel("Size: ... KB");

	requestProgressBar = new QProgressBar();
	requestProgressBar->setObjectName(QStringLiteral("FetchProgress"));
	requestProgressBar->setMaximumWidth(120); // Фиксированная небольшая ширина
	requestProgressBar->setMaximumHeight(16);
	requestProgressBar->setTextVisible(false); // Скрываем текст "XX%" внутри бара для компактности

	statusLayout->addWidget(statusLabel);
	statusLayout->addStretch();
	statusLayout->addWidget(timeLabel);
	statusLayout->addWidget(sizeLabel);
	statusLayout->addWidget(requestProgressBar);

	// Вкладки ответа
	QTabWidget* respTabs = new QTabWidget();

	auto connamdController = commandContext->commandController();
	responseBodyEdit = new CodeEditorWidget(
		connamdController->services()->get<HighlightingPluginManager>(),
		connamdController->services()->get<FormatterPluginManager>());
	responseBodyEdit->setObjectName(QStringLiteral("FetchResponseBody"));
	responseBodyEdit->setReadOnly(true);
	responseBodyEdit->setFont(monoFont);
	responseBodyEdit->setPlaceholderText("Ответ сервера появится здесь...");

	responseCoocies = new KeyValueEditorWidget();
	responseCoocies->setReadonly(true);

	responseHeaders = new KeyValueEditorWidget();
	responseHeaders->setReadonly(true);

	respTabs->addTab(responseBodyEdit, "Body");
	respTabs->addTab(responseHeaders, "Headers");
	respTabs->addTab(responseCoocies, "Cookies");

	respLayout->addLayout(statusLayout);
	respLayout->addWidget(respTabs);

	// Добавляем виджеты в разделитель
	splitter->addWidget(requestWidget);
	splitter->addWidget(responseWidget);

	// Задаем начальные пропорции (50% на запрос, 50% на ответ)
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);

	mainLayout->addLayout(topBarLayout);
	mainLayout->addWidget(splitter, 1);
}

void FetchApiWidget::onSendClicked() {
	d->requestTimer.start();
	d->requestProgressBar->setValue(0);
	d->timeLabel->setText("Time: ... ms");
	d->sizeLabel->setText("Size: ... KB");
	d->responseBodyEdit->clear();

	FetchApiOpt opt(
		d->requestUrlEdit->text(),
		d->requestParams->parametersMap(),
		d->requestHeaders->parametersMap());
	if (!opt.setMethod(d->requestMethodCombo->currentText().toLower())) {
		return;
	}

	opt.setBody(d->requestBodyEdit->toPlainText().toUtf8());

	d->fetchApiService->fetchRequest(opt,
		[this](int statusCode, const QByteArray& data, const QHttpHeaders& headers) { d->fetchSuccess(statusCode, data, headers);	},
		[this](int errorCode, const QString& errorString, const QHttpHeaders& headers) { d->fetchError(errorCode, errorString, headers); },
		[this](qint64 received, qint64 total) { d->fetchProgress(received, total); });
}

void FetchApiWidget::Private::fetchSuccess(int statusCode, const QByteArray& data, const QHttpHeaders& headers) {
	responseBodyEdit->setContentType(QString::fromUtf8(headers.value("Content-Type")));
	responseBodyEdit->setText(QString::fromUtf8(data));
	responseBodyEdit->formatDocument();

	requestProgressBar->setProperty("state", QString());
	refreshStyle(requestProgressBar);

	qint64 elapsedMs = requestTimer.elapsed();
	timeLabel->setText(QString("Time: %1 ms").arg(elapsedMs));
	sizeLabel->setText(QString("Size: %1").arg(formatBytes(data.size())));

	setResponseHeaders(headers);
	setStatus(statusCode, "OK");
}

void FetchApiWidget::Private::fetchError(int errorCode, const QString& errorString, const QHttpHeaders& headers) {
	requestProgressBar->setRange(0, 100);
	requestProgressBar->setValue(100);
	requestProgressBar->setProperty("state", QStringLiteral("error"));
	refreshStyle(requestProgressBar);

	qint64 elapsedMs = requestTimer.elapsed();
	timeLabel->setText(QString("Time: %1 ms").arg(elapsedMs));

	setResponseHeaders(headers);
	setStatus(errorCode, errorString);
}

void FetchApiWidget::Private::fetchProgress(qint64 received, qint64 total) {
	if (total > 0) {
		// Известен общий размер: обычный прогресс-бар
		requestProgressBar->setRange(0, total);
		requestProgressBar->setValue(received);

		// Обновляем метку размера в реальном времени
		sizeLabel->setText(QString("Size: %1 / %2")
			.arg(formatBytes(received))
			.arg(formatBytes(total)));
	}
	else {
		// Размер неизвестен: включаем "бесконечный" (indeterminate) режим
		// В Qt это делается установкой range от 0 до 0, что показывает бегущую полоску
		requestProgressBar->setRange(0, 0);
		sizeLabel->setText(QString("Size: %1 (unknown total)")
			.arg(formatBytes(received)));
	}
}

void FetchApiWidget::Private::setStatus(int statusCode, const QString& message) {
	const bool success = (statusCode == 200);
	statusLabel->setProperty("state", success ? QStringLiteral("success") : QStringLiteral("error"));
	refreshStyle(statusLabel);

	statusLabel->setText(QString("%1 %2")
		.arg(statusCode)
		.arg(message));
}

QString FetchApiWidget::Private::formatBytes(qint64 bytes) const {
	if (bytes < 1024) return QString("%1 B").arg(bytes);
	if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
	return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
}

void FetchApiWidget::Private::setResponseHeaders(const QHttpHeaders& headers) {
	std::vector<KeyValueEditorWidget::Parameter> parameters;
	for (qsizetype i = 0; i < headers.size(); i++) {
		KeyValueEditorWidget::Parameter parameter;
		parameter.name = headers.nameAt(i);
		parameter.value = QString::fromUtf8(headers.valueAt(i));

		parameters.push_back(parameter);
	}

	responseHeaders->setParameters(parameters);
}
