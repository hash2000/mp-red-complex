#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
#include "Content/FetchApiModule/models/fetch_opt.h"
#include "Content/FetchApiModule/services/fetch_service.h"

#include "Libs/BaseWidgets/extensions/text_edit_extensions.h"
#include "Libs/BaseWidgets/key_value/key_value_editor_widget.h"

#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QSplitter>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>

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

/// Ответ в формате JSON показывается с отступами, остальное — как есть.
QString prettyResponseText(const FetchApiResult& result) {
	const auto text = result.text();
	if (!result.contentType().contains(QStringLiteral("json"), Qt::CaseInsensitive)) {
		return text;
	}

	QJsonParseError error;
	const auto document = QJsonDocument::fromJson(text.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError || document.isNull()) {
		return text;
	}

	return QString::fromUtf8(document.toJson(QJsonDocument::Indented));
}

} // namespace

class FetchApiWidget::Private {
public:
	/// Тип авторизации; значение хранится в данных элемента комбобокса.
	enum AuthType {
		AuthNone = 0,
		AuthBasic,
		AuthBearer,
		AuthApiKey,
	};

	Private(FetchApiWidget* parent) : q(parent) { }
	FetchApiWidget* q;

	FetchApiService* service = nullptr;

	QComboBox* methodCombo = nullptr;
	QLineEdit* urlEdit = nullptr;

	/// Параметры отправки, которыми управляет документ через панель свойств.
	FetchApiHandlerType handlerType = FetchApiHandlerType::Rest;
	FetchApiSslPolicy sslPolicyValue = FetchApiSslPolicy::Strict;
	QString caFileValue;

	QPlainTextEdit* bodyEdit = nullptr;
	KeyValueEditorWidget* headersEditor = nullptr;
	KeyValueEditorWidget* paramsEditor = nullptr;
	QComboBox* authTypeCombo = nullptr;
	QLineEdit* authNameEdit = nullptr;
	QLineEdit* authValueEdit = nullptr;
	QLabel* authNameLabel = nullptr;
	QLabel* authValueLabel = nullptr;
	QComboBox* authAddToCombo = nullptr;

	/// Ответ — простое текстовое поле без подсветки.
	QPlainTextEdit* responseBody = nullptr;
	KeyValueEditorWidget* responseHeaders = nullptr;
	KeyValueEditorWidget* responseCookies = nullptr;
	QLabel* statusLabel = nullptr;
	QLabel* timeLabel = nullptr;
	QLabel* sizeLabel = nullptr;
	QProgressBar* progressBar = nullptr;

	bool busy = false;

	void setupUi();
	void updateAuthControls();
	void setBusy(bool value);

	FetchApiMethod currentMethod() const;

	void applyAuth(FetchApiOpt& options) const;

	void setResponseText(const QString& text);
	void clearResponseText();
	void setResponseHeaders(const QHttpHeaders& headers);
	void setResponseCookies(const QList<QPair<QString, QString>>& cookies);

	static FetchApiKeyValues editorKeyValues(const KeyValueEditorWidget* editor);
	static std::vector<KeyValueEditorWidget::Parameter> toParameters(
		const FetchApiKeyValues& values);
	static QFont monoFont();
	static QString formatBytes(qint64 bytes);
};

FetchApiWidget::FetchApiWidget(FetchApiService* service, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->service = service;
	d->setupUi();

	if (!d->service) {
		d->statusLabel->setProperty("state", QStringLiteral("error"));
		refreshStyle(d->statusLabel);
		d->statusLabel->setText(tr("HTTP-сервис недоступен"));
	}
}

FetchApiWidget::~FetchApiWidget() = default;

void FetchApiWidget::Private::setupUi() {
	auto* mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(15, 15, 15, 15);
	mainLayout->setSpacing(10);

	// --- Строка запроса: метод и адрес -------------------------------------
	// Отправка и её параметры (тип обработчика, TLS) живут в ленте документа
	// и панели «Свойства».
	auto* topBar = new QHBoxLayout();

	methodCombo = new QComboBox();
	methodCombo->setObjectName(QStringLiteral("FetchMethodCombo"));
	methodCombo->addItem(QStringLiteral("GET"), static_cast<int>(FetchApiMethod::Get));
	methodCombo->addItem(QStringLiteral("POST"), static_cast<int>(FetchApiMethod::Post));
	methodCombo->addItem(QStringLiteral("PUT"), static_cast<int>(FetchApiMethod::Put));
	methodCombo->addItem(QStringLiteral("DELETE"), static_cast<int>(FetchApiMethod::Delete));
	methodCombo->addItem(QStringLiteral("PATCH"), static_cast<int>(FetchApiMethod::Patch));
	methodCombo->addItem(QStringLiteral("HEAD"), static_cast<int>(FetchApiMethod::Head));
	methodCombo->setMinimumWidth(110);

	urlEdit = new QLineEdit();
	urlEdit->setObjectName(QStringLiteral("FetchUrlEdit"));
	urlEdit->setClearButtonEnabled(true);
	urlEdit->setPlaceholderText(QStringLiteral(
		"https://api.example.com/v1/users"));

	topBar->addWidget(methodCombo);
	topBar->addWidget(urlEdit, 1);

	// --- Область запроса ----------------------------------------------------
	auto* requestPanel = new QWidget();
	auto* requestLayout = new QVBoxLayout(requestPanel);
	requestLayout->setContentsMargins(0, 0, 0, 0);

	auto* requestTabs = new QTabWidget();
	requestTabs->setObjectName(QStringLiteral("FetchRequestTabs"));

	bodyEdit = new QPlainTextEdit();
	bodyEdit->setObjectName(QStringLiteral("FetchBodyEdit"));
	bodyEdit->setPlaceholderText(QStringLiteral("{\n  \"key\": \"value\"\n}"));
	bodyEdit->setFont(monoFont());
	Extensions::TextEdit::setTabDistance(bodyEdit, 2);

	headersEditor = new KeyValueEditorWidget();
	headersEditor->setObjectName(QStringLiteral("FetchRequestHeaders"));

	paramsEditor = new KeyValueEditorWidget();
	paramsEditor->setObjectName(QStringLiteral("FetchRequestParams"));

	auto* authPanel = new QWidget();
	authPanel->setObjectName(QStringLiteral("FetchRequestAuth"));
	auto* authForm = new QFormLayout(authPanel);

	authTypeCombo = new QComboBox();
	authTypeCombo->setObjectName(QStringLiteral("FetchAuthTypeCombo"));
	authTypeCombo->addItem(QStringLiteral("Без авторизации"), Private::AuthNone);
	authTypeCombo->addItem(QStringLiteral("Basic"), Private::AuthBasic);
	authTypeCombo->addItem(QStringLiteral("Bearer token"), Private::AuthBearer);
	authTypeCombo->addItem(QStringLiteral("API key"), Private::AuthApiKey);

	authNameEdit = new QLineEdit();
	authNameEdit->setObjectName(QStringLiteral("FetchAuthNameEdit"));

	authValueEdit = new QLineEdit();
	authValueEdit->setObjectName(QStringLiteral("FetchAuthValueEdit"));

	authNameLabel = new QLabel(QStringLiteral("Логин:"));
	authValueLabel = new QLabel(QStringLiteral("Пароль:"));

	authAddToCombo = new QComboBox();
	authAddToCombo->setObjectName(QStringLiteral("FetchAuthAddToCombo"));
	authAddToCombo->addItem(QStringLiteral("Заголовок"), 0);
	authAddToCombo->addItem(QStringLiteral("Query-параметр"), 1);

	authForm->addRow(QStringLiteral("Тип:"), authTypeCombo);
	authForm->addRow(authNameLabel, authNameEdit);
	authForm->addRow(authValueLabel, authValueEdit);
	authForm->addRow(QStringLiteral("Передавать как:"), authAddToCombo);

	requestTabs->addTab(bodyEdit, QStringLiteral("Body"));
	requestTabs->addTab(headersEditor, QStringLiteral("Headers"));
	requestTabs->addTab(paramsEditor, QStringLiteral("Params"));
	requestTabs->addTab(authPanel, QStringLiteral("Auth"));

	requestLayout->addWidget(requestTabs);

	// --- Область ответа -----------------------------------------------------
	auto* responsePanel = new QWidget();
	auto* responseLayout = new QVBoxLayout(responsePanel);
	responseLayout->setContentsMargins(0, 0, 0, 0);

	auto* statusBar = new QHBoxLayout();

	statusLabel = new QLabel();
	statusLabel->setObjectName(QStringLiteral("FetchStatus"));
	statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	timeLabel = new QLabel(QStringLiteral("Time: —"));
	timeLabel->setObjectName(QStringLiteral("FetchTime"));

	sizeLabel = new QLabel(QStringLiteral("Size: —"));
	sizeLabel->setObjectName(QStringLiteral("FetchSize"));

	progressBar = new QProgressBar();
	progressBar->setObjectName(QStringLiteral("FetchProgress"));
	progressBar->setRange(0, 100);
	progressBar->setValue(0);
	progressBar->setMaximumWidth(120);
	progressBar->setMaximumHeight(16);
	progressBar->setTextVisible(false);

	statusBar->addWidget(statusLabel, 1);
	statusBar->addWidget(timeLabel);
	statusBar->addWidget(sizeLabel);
	statusBar->addWidget(progressBar);

	auto* responseTabs = new QTabWidget();
	responseTabs->setObjectName(QStringLiteral("FetchResponseTabs"));

	// Ответ показывается в простом текстовом поле: модуль не зависит от
	// редактора кода и его подсветки.
	responseBody = new QPlainTextEdit();
	responseBody->setObjectName(QStringLiteral("FetchResponseBody"));
	responseBody->setReadOnly(true);
	responseBody->setFont(monoFont());
	responseBody->setPlaceholderText(QStringLiteral("Ответ сервера появится здесь..."));

	responseHeaders = new KeyValueEditorWidget();
	responseHeaders->setObjectName(QStringLiteral("FetchResponseHeaders"));
	responseHeaders->setReadonly(true);

	responseCookies = new KeyValueEditorWidget();
	responseCookies->setObjectName(QStringLiteral("FetchResponseCookies"));
	responseCookies->setReadonly(true);

	responseTabs->addTab(responseBody, QStringLiteral("Body"));
	responseTabs->addTab(responseHeaders, QStringLiteral("Headers"));
	responseTabs->addTab(responseCookies, QStringLiteral("Cookies"));

	responseLayout->addLayout(statusBar);
	responseLayout->addWidget(responseTabs);

	auto* splitter = new QSplitter(Qt::Vertical);
	splitter->addWidget(requestPanel);
	splitter->addWidget(responsePanel);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);

	mainLayout->addLayout(topBar);
	mainLayout->addWidget(splitter, 1);

	// --- Связи --------------------------------------------------------------
	QObject::connect(urlEdit, &QLineEdit::returnPressed, q, &FetchApiWidget::sendRequest);
	QObject::connect(authTypeCombo, &QComboBox::currentIndexChanged, q,
		[this](int) { updateAuthControls(); });

	updateAuthControls();
}

void FetchApiWidget::Private::updateAuthControls() {
	const int type = authTypeCombo->currentData().toInt();

	const bool showName = type != AuthNone;
	const bool showValue = type == AuthBasic || type == AuthApiKey;

	authNameLabel->setVisible(showName);
	authNameEdit->setVisible(showName);
	authValueLabel->setVisible(showValue);
	authValueEdit->setVisible(showValue);
	authAddToCombo->setVisible(type == AuthApiKey);

	switch (type) {
	case AuthBasic:
		authNameLabel->setText(QStringLiteral("Логин:"));
		authValueLabel->setText(QStringLiteral("Пароль:"));
		authNameEdit->setPlaceholderText(QStringLiteral("user"));
		authValueEdit->setPlaceholderText(QStringLiteral("password"));
		authValueEdit->setEchoMode(QLineEdit::Password);
		break;
	case AuthBearer:
		authNameLabel->setText(QStringLiteral("Токен:"));
		authNameEdit->setPlaceholderText(QStringLiteral("токен доступа"));
		authValueEdit->setEchoMode(QLineEdit::Normal);
		break;
	case AuthApiKey:
		authNameLabel->setText(QStringLiteral("Ключ:"));
		authValueLabel->setText(QStringLiteral("Значение:"));
		authNameEdit->setPlaceholderText(QStringLiteral("X-API-Key"));
		authValueEdit->setPlaceholderText(QStringLiteral("значение ключа"));
		authValueEdit->setEchoMode(QLineEdit::Normal);
		break;
	default:
		authValueEdit->setEchoMode(QLineEdit::Normal);
		break;
	}
}

void FetchApiWidget::Private::setBusy(bool value) {
	busy = value;
	methodCombo->setEnabled(!value);
	urlEdit->setReadOnly(value);

	if (value) {
		progressBar->setRange(0, 0);
	}
	else {
		progressBar->setRange(0, 100);
	}
}

FetchApiMethod FetchApiWidget::Private::currentMethod() const {
	const auto value = methodCombo->currentData().toInt();
	return static_cast<FetchApiMethod>(value);
}

void FetchApiWidget::Private::applyAuth(FetchApiOpt& options) const {
	const int type = authTypeCombo->currentData().toInt();
	const auto name = authNameEdit->text().trimmed();
	const auto value = authValueEdit->text();

	switch (type) {
	case AuthBasic: {
		if (name.isEmpty()) {
			return;
		}

		const auto raw = QStringLiteral("%1:%2").arg(name, value).toUtf8();
		options.addHeader(QStringLiteral("Authorization"),
			QStringLiteral("Basic %1").arg(QString::fromLatin1(raw.toBase64())));
		break;
	}
	case AuthBearer: {
		if (name.isEmpty()) {
			return;
		}

		options.addHeader(QStringLiteral("Authorization"),
			QStringLiteral("Bearer %1").arg(name));
		break;
	}
	case AuthApiKey: {
		const auto key = name.isEmpty() ? QStringLiteral("X-API-Key") : name;
		if (authAddToCombo->currentData().toInt() == 1) {
			options.addParameter(key, value);
		}
		else {
			options.addHeader(key, value);
		}
		break;
	}
	default:
		break;
	}
}

void FetchApiWidget::Private::setResponseText(const QString& text) {
	responseBody->setPlainText(text);
}

void FetchApiWidget::Private::clearResponseText() {
	responseBody->clear();
}

void FetchApiWidget::Private::setResponseHeaders(const QHttpHeaders& headers) {
	std::vector<KeyValueEditorWidget::Parameter> parameters;
	parameters.reserve(static_cast<size_t>(headers.size()));
	for (qsizetype i = 0; i < headers.size(); i++) {
		KeyValueEditorWidget::Parameter parameter;
		parameter.name = FetchApiFormat::headerName(QString::fromLatin1(headers.nameAt(i)));
		parameter.value = QString::fromUtf8(headers.valueAt(i));
		parameter.description.clear();
		parameter.isEnabled = true;
		parameters.push_back(parameter);
	}

	responseHeaders->setParameters(parameters);
}

void FetchApiWidget::Private::setResponseCookies(const QList<QPair<QString, QString>>& cookies) {
	std::vector<KeyValueEditorWidget::Parameter> parameters;
	parameters.reserve(static_cast<size_t>(cookies.size()));
	for (const auto& [name, attributes] : cookies) {
		KeyValueEditorWidget::Parameter parameter;
		parameter.name = name;
		parameter.value = attributes;
		parameter.description.clear();
		parameter.isEnabled = true;
		parameters.push_back(parameter);
	}

	responseCookies->setParameters(parameters);
}

FetchApiKeyValues FetchApiWidget::Private::editorKeyValues(const KeyValueEditorWidget* editor) {
	FetchApiKeyValues values;
	if (!editor) {
		return values;
	}

	for (const auto& parameter : editor->parameters()) {
		if (!parameter.isEnabled) {
			continue;
		}

		const auto name = parameter.name.trimmed();
		if (name.isEmpty()) {
			continue;
		}

		values[name] = parameter.value;
	}

	return values;
}

std::vector<KeyValueEditorWidget::Parameter> FetchApiWidget::Private::toParameters(
	const FetchApiKeyValues& values) {
	std::vector<KeyValueEditorWidget::Parameter> parameters;
	parameters.reserve(values.size());
	for (const auto& [name, value] : values) {
		KeyValueEditorWidget::Parameter parameter;
		parameter.name = name;
		parameter.value = value;
		parameter.isEnabled = true;
		parameters.push_back(parameter);
	}

	return parameters;
}

QFont FetchApiWidget::Private::monoFont() {
	QFont font(QStringLiteral("Consolas"), 11);
	if (!font.exactMatch()) {
		font = QFont(QStringLiteral("Courier New"), 11);
	}

	return font;
}

QString FetchApiWidget::Private::formatBytes(qint64 bytes) {
	if (bytes < 1024) {
		return QStringLiteral("%1 B").arg(bytes);
	}
	if (bytes < 1024 * 1024) {
		return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
	}
	return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
}

FetchApiOpt FetchApiWidget::requestOptions() const {
	FetchApiOpt options(location(),
		Private::editorKeyValues(d->paramsEditor),
		Private::editorKeyValues(d->headersEditor));
	options.setMethod(d->currentMethod());
	options.setBodyText(bodyText());
	options.setSslPolicy(d->sslPolicyValue);
	options.setHandler(d->handlerType);

	if (options.sslPolicy() == FetchApiSslPolicy::TrustCustomCa) {
		const auto path = caFile();
		if (!path.isEmpty()) {
			QString error;
			if (!options.loadCustomCaFromFile(path, &error)) {
				qWarning() << "FetchApiWidget:" << error;
			}
		}
	}

	d->applyAuth(options);
	return options;
}

QString FetchApiWidget::location() const {
	return d->urlEdit->text().trimmed();
}

void FetchApiWidget::setLocation(const QString& location) {
	d->urlEdit->setText(location);
}

FetchApiMethod FetchApiWidget::method() const {
	return d->currentMethod();
}

void FetchApiWidget::setMethod(FetchApiMethod method) {
	const auto index = d->methodCombo->findData(static_cast<int>(method));
	if (index >= 0) {
		d->methodCombo->setCurrentIndex(index);
	}
}

QString FetchApiWidget::bodyText() const {
	return d->bodyEdit->toPlainText();
}

void FetchApiWidget::setBodyText(const QString& text) {
	if (d->bodyEdit->toPlainText() == text) {
		return;
	}

	d->bodyEdit->setPlainText(text);
}

QString FetchApiWidget::parametersFormatted() const {
	return FetchApiFormat::formatKeyValues(Private::editorKeyValues(d->paramsEditor));
}

void FetchApiWidget::setParametersFormatted(const QString& parameters) {
	d->paramsEditor->setParameters(Private::toParameters(
		FetchApiFormat::parseKeyValues(parameters)));
}

QString FetchApiWidget::headersFormatted() const {
	return FetchApiFormat::formatKeyValues(Private::editorKeyValues(d->headersEditor));
}

void FetchApiWidget::setHeadersFormatted(const QString& headers) {
	d->headersEditor->setParameters(Private::toParameters(
		FetchApiFormat::parseKeyValues(headers)));
}

FetchApiSslPolicy FetchApiWidget::sslPolicy() const {
	return d->sslPolicyValue;
}

void FetchApiWidget::setSslPolicy(FetchApiSslPolicy policy) {
	d->sslPolicyValue = policy;
}

QString FetchApiWidget::caFile() const {
	return d->caFileValue;
}

void FetchApiWidget::setCaFile(const QString& path) {
	d->caFileValue = path;
}

FetchApiHandlerType FetchApiWidget::handler() const {
	return d->handlerType;
}

void FetchApiWidget::setHandler(FetchApiHandlerType handler) {
	d->handlerType = handler;
}

bool FetchApiWidget::isBusy() const {
	return d->busy;
}

void FetchApiWidget::sendRequest() {
	if (d->busy) {
		return;
	}

	if (!d->service) {
		showError(tr("HTTP-сервис недоступен"));
		emit requestFailed(tr("HTTP-сервис недоступен"));
		return;
	}

	auto options = requestOptions();
	if (!options.isValid()) {
		const auto message = location().isEmpty()
			? tr("Не задан адрес запроса")
			: tr("Некорректный адрес запроса: %1").arg(location());
		showError(message);
		emit requestFailed(message);
		return;
	}

	clearResult();
	d->setBusy(true);
	d->statusLabel->setProperty("state", QString());
	refreshStyle(d->statusLabel);
	d->statusLabel->setText(tr("Отправка: %1").arg(options.describe()));
	emit requestStarted();

	QPointer<FetchApiWidget> self(this);
	const bool started = d->service->execute(options,
		[self](const FetchApiResult& result) {
			if (!self) {
				return;
			}

			self->d->setBusy(false);
			self->showResult(result);
			emit self->requestFinished(result);
			if (result.failed) {
				emit self->requestFailed(result.errorMessage());
			}
		},
		[self](qint64 received, qint64 total) {
			if (!self) {
				return;
			}

			self->showProgress(received, total);
		});

	if (!started) {
		const auto message = d->service->lastError();
		showError(message);
		emit requestFailed(message);
	}
}

void FetchApiWidget::showResult(const FetchApiResult& result) {
	d->setBusy(false);

	d->statusLabel->setProperty("state",
		result.failed ? QStringLiteral("error") : QStringLiteral("success"));
	refreshStyle(d->statusLabel);

	const auto status = result.statusCode > 0
		? QStringLiteral("%1 %2").arg(result.statusCode).arg(result.statusText).trimmed()
		: tr("Ошибка");
	d->statusLabel->setText(status);
	d->statusLabel->setToolTip(result.failed
		? result.errorMessage()
		: result.url.toString());

	d->timeLabel->setText(tr("Time: %1 ms").arg(result.elapsedMs));
	d->sizeLabel->setText(tr("Size: %1").arg(Private::formatBytes(result.size)));

	auto text = prettyResponseText(result);
	if (result.failed && text.isEmpty()) {
		text = result.errorMessage();
	}
	d->setResponseText(text);
	d->setResponseHeaders(result.headers);
	d->setResponseCookies(result.cookies());

	d->progressBar->setRange(0, 100);
	d->progressBar->setValue(100);
	d->progressBar->setProperty("state",
		result.failed ? QStringLiteral("error") : QString());
	refreshStyle(d->progressBar);
}

void FetchApiWidget::showError(const QString& message) {
	d->setBusy(false);

	d->statusLabel->setProperty("state", QStringLiteral("error"));
	refreshStyle(d->statusLabel);
	d->statusLabel->setText(tr("Ошибка"));
	d->statusLabel->setToolTip(message);

	d->timeLabel->setText(QStringLiteral("Time: —"));
	d->sizeLabel->setText(QStringLiteral("Size: —"));

	d->setResponseText(message);
	d->setResponseHeaders(QHttpHeaders());
	d->setResponseCookies({});

	d->progressBar->setRange(0, 100);
	d->progressBar->setValue(100);
	d->progressBar->setProperty("state", QStringLiteral("error"));
	refreshStyle(d->progressBar);
}

void FetchApiWidget::showProgress(qint64 received, qint64 total) {
	if (total > 0) {
		d->progressBar->setRange(0, static_cast<int>(total));
		d->progressBar->setValue(static_cast<int>(received));
		d->sizeLabel->setText(tr("Size: %1 / %2")
			.arg(Private::formatBytes(received), Private::formatBytes(total)));
		return;
	}

	// Размер неизвестен: бегущая полоска (0..0) и растущий счётчик.
	d->progressBar->setRange(0, 0);
	d->sizeLabel->setText(tr("Size: %1").arg(Private::formatBytes(received)));
}

void FetchApiWidget::clearResult() {
	d->clearResponseText();
	d->setResponseHeaders(QHttpHeaders());
	d->setResponseCookies({});

	d->statusLabel->setText(QString());
	d->statusLabel->setToolTip(QString());
	d->statusLabel->setProperty("state", QString());
	refreshStyle(d->statusLabel);

	d->timeLabel->setText(QStringLiteral("Time: —"));
	d->sizeLabel->setText(QStringLiteral("Size: —"));

	d->progressBar->setRange(0, 100);
	d->progressBar->setValue(0);
	d->progressBar->setProperty("state", QString());
	refreshStyle(d->progressBar);
}
