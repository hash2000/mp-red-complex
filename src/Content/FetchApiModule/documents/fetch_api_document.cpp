#include "Content/FetchApiModule/documents/fetch_api_document.h"
#include "Content/FetchApiModule/models/fetch_opt.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/widgets/fetch_api_widget.h"

#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/ribbon/document_ribbon.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <utility>

namespace {

/// Идентификаторы свойств панели «Свойства» в группе «Параметры отправки».
const QString kHandlerProperty = QStringLiteral("handler");
const QString kSslPolicyProperty = QStringLiteral("ssl-policy");
const QString kSslCaFileProperty = QStringLiteral("ssl-ca-file");

/// Идентификатор группы свойств, которую раскрывает значок у кнопки
/// «Отправить».
const QString kRequestGroup = QStringLiteral("request");

/// Прочитать метод запроса из контекста; некорректное значение игнорируется.
std::optional<FetchApiMethod> methodFromContext(const QVariant& value) {
	return From<FetchApiMethod>::from(value.toString());
}

std::optional<FetchApiSslPolicy> sslPolicyFromContext(const QVariant& value) {
	return From<FetchApiSslPolicy>::from(value.toString());
}

std::optional<FetchApiHandlerType> handlerFromContext(const QVariant& value) {
	return From<FetchApiHandlerType>::from(value.toString());
}

/// Подпись типа обработчика в панели свойств (сейчас реализован только REST).
QString handlerTitle(FetchApiHandlerType handler) {
	return handler == FetchApiHandlerType::Rest ? QStringLiteral("REST") : QString();
}

QString sslPolicyTitle(FetchApiSslPolicy policy) {
	switch (policy) {
	case FetchApiSslPolicy::Strict:
		return QStringLiteral("Strict");
	case FetchApiSslPolicy::TrustCustomCa:
		return QStringLiteral("Trust custom CA");
	case FetchApiSslPolicy::IgnoreErrors:
		return QStringLiteral("Ignore errors");
	}

	return QStringLiteral("Strict");
}

std::optional<FetchApiSslPolicy> sslPolicyFromTitle(const QString& title) {
	if (title == QStringLiteral("Strict")) {
		return FetchApiSslPolicy::Strict;
	}
	if (title == QStringLiteral("Trust custom CA")) {
		return FetchApiSslPolicy::TrustCustomCa;
	}
	if (title == QStringLiteral("Ignore errors")) {
		return FetchApiSslPolicy::IgnoreErrors;
	}

	return From<FetchApiSslPolicy>::from(title);
}

} // namespace

struct FetchApiDocument::Private {
	FetchApiDocument* q = nullptr;

	FetchApiWidget* widget = nullptr;
	DocumentRibbon* ribbon = nullptr;
	RibbonButton* sendButton = nullptr;

	/// true — документ сам обновляет виджет или контекст: обратная реакция
	/// на изменение переменных в это время не нужна.
	bool applying = false;

	QWidget* createRequestPropertiesEditor(QWidget* parent);
	QList<PropertyData> requestProperties() const;
	void applyRequestProperty(const QString& id, const QVariant& value);
	void writeContextVariable(const char* name, const QVariant& value);
};

FetchApiDocument::FetchApiDocument(FetchApiService* service,
	HighlightingPluginManager* highlighting,
	FormatterPluginManager* formatters,
	QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>()) {
	d->q = this;
	setGlyph(QStringLiteral("🌐"));
	setTitle(tr("Fetch API"));

	d->widget = new FetchApiWidget(service, highlighting, formatters, this);

	// Лента документа содержит функцию «Отправить»; её параметры
	// (тип запроса и обработка TLS) редактируются в панели «Свойства».
	d->ribbon = new DocumentRibbon(this);
	auto* requestGroup = d->ribbon->addGroup(tr("Запрос"));
	d->sendButton = d->ribbon->addButton(requestGroup, tr("Отправить"),
		tr("Отправить запрос"), kRequestGroup);
	connect(d->sendButton, &RibbonButton::clicked, this, [this] {
		if (d->widget) {
			d->widget->sendRequest();
		}
	});
	connect(d->ribbon, &DocumentRibbon::propertiesRequested, this,
		[this](const QString& groupId) {
			showProperties(groupId);
		});

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(d->ribbon, 0);
	layout->addWidget(d->widget, 1);

	// Параметры запроса попадают в контекст перед отправкой, поэтому даже
	// после перезапуска документ можно восстановить из переменных.
	connect(d->widget, &FetchApiWidget::requestStarted,
		this, &FetchApiDocument::writeRequestToContext);
	connect(d->widget, &FetchApiWidget::requestStarted, this, [this] {
		if (d->sendButton) {
			d->sendButton->setEnabled(false);
		}
	});
	connect(d->widget, &FetchApiWidget::requestFinished,
		this, &FetchApiDocument::onRequestFinished);
	connect(d->widget, &FetchApiWidget::requestFinished, this, [this](const FetchApiResult&) {
		if (d->sendButton) {
			d->sendButton->setEnabled(true);
		}
	});
	connect(d->widget, &FetchApiWidget::requestFailed, this,
		[this](const QString& message) {
			if (d->sendButton) {
				d->sendButton->setEnabled(true);
			}
			printError(message);
		});
}

FetchApiDocument::~FetchApiDocument() = default;

FetchApiWidget* FetchApiDocument::widget() const {
	return d->widget;
}

void FetchApiDocument::Private::writeContextVariable(const char* name, const QVariant& value) {
	applying = true;
	q->setVariable(QString::fromLatin1(name), value);
	applying = false;
}

QList<PropertyData> FetchApiDocument::Private::requestProperties() const {
	QList<PropertyData> result;
	if (!widget) {
		return result;
	}

	PropertyData handler;
	handler.id = kHandlerProperty;
	handler.label = q->tr("Тип запроса");
	handler.type = PropertyType::Combo;
	handler.comboOptions = { handlerTitle(FetchApiHandlerType::Rest) };
	handler.value = handlerTitle(widget->handler());
	handler.tooltip = q->tr("Тип обработчика запроса. Сейчас реализован только REST.");
	result << handler;

	PropertyData policy;
	policy.id = kSslPolicyProperty;
	policy.label = q->tr("Политика TLS");
	policy.type = PropertyType::Combo;
	policy.comboOptions = {
		sslPolicyTitle(FetchApiSslPolicy::Strict),
		sslPolicyTitle(FetchApiSslPolicy::TrustCustomCa),
		sslPolicyTitle(FetchApiSslPolicy::IgnoreErrors),
	};
	policy.value = sslPolicyTitle(widget->sslPolicy());
	policy.tooltip = q->tr("Strict — проверять сертификаты, Trust custom CA — "
		"доверять только указанному файлу, Ignore errors — не проверять TLS.");
	result << policy;

	PropertyData caFile;
	caFile.id = kSslCaFileProperty;
	caFile.label = q->tr("Файл CA-сертификата");
	caFile.type = PropertyType::PathFile;
	caFile.value = widget->caFile();
	caFile.fileFilter = q->tr("Сертификаты (*.pem *.crt *.cer *.der);;Все файлы (*.*)");
	caFile.tooltip = q->tr("Используется при политике «Trust custom CA».");
	result << caFile;

	return result;
}

QWidget* FetchApiDocument::Private::createRequestPropertiesEditor(QWidget* parent) {
	auto* properties = new PropertiesListWidget(parent);
	properties->setObjectName(QStringLiteral("FetchRequestProperties"));
	properties->setHeaderVisible(false);
	properties->setProperties(requestProperties());

	connect(properties, &PropertiesListWidget::propertyChanged, q,
		[this](const QString& id, const QVariant& value) {
			applyRequestProperty(id, value);
		});

	return properties;
}

void FetchApiDocument::Private::applyRequestProperty(const QString& id, const QVariant& value) {
	if (!widget) {
		return;
	}

	if (id == kHandlerProperty) {
		const auto handler = From<FetchApiHandlerType>::from(value.toString());
		if (!handler) {
			return;
		}

		widget->setHandler(*handler);
		writeContextVariable(FetchApiVariables::kHandler,
			Format<FetchApiHandlerType>::format(*handler));
		return;
	}

	if (id == kSslPolicyProperty) {
		const auto policy = sslPolicyFromTitle(value.toString());
		if (!policy) {
			return;
		}

		widget->setSslPolicy(*policy);
		writeContextVariable(FetchApiVariables::kSslPolicy,
			Format<FetchApiSslPolicy>::format(*policy));
		return;
	}

	if (id == kSslCaFileProperty) {
		widget->setCaFile(value.toString());
		writeContextVariable(FetchApiVariables::kSslCaFile, value.toString());
	}
}

void FetchApiDocument::applyContextToWidget() {
	if (!context() || !d->widget) {
		return;
	}

	d->applying = true;

	if (hasVariable(FetchApiVariables::kMethod)) {
		if (const auto method = methodFromContext(variable(FetchApiVariables::kMethod))) {
			d->widget->setMethod(*method);
		}
	}

	if (hasVariable(FetchApiVariables::kLocation)) {
		d->widget->setLocation(variable(FetchApiVariables::kLocation).toString());
	}
	else if (hasVariable(FetchApiVariables::kUrlAlias)) {
		d->widget->setLocation(variable(FetchApiVariables::kUrlAlias).toString());
	}

	if (hasVariable(FetchApiVariables::kBody)) {
		d->widget->setBodyText(variable(FetchApiVariables::kBody).toString());
	}

	if (hasVariable(FetchApiVariables::kParams)) {
		d->widget->setParametersFormatted(
			variable(FetchApiVariables::kParams).toString());
	}
	else if (hasVariable(FetchApiVariables::kParamsAlias)) {
		d->widget->setParametersFormatted(
			variable(FetchApiVariables::kParamsAlias).toString());
	}

	// `headers` при привязке контекста — заголовки запроса. После ответа
	// в этой же переменной живут заголовки ответа (требование модуля),
	// поэтому обратная синхронизация по ней ниже не выполняется.
	if (hasVariable(FetchApiVariables::kHeaders)) {
		d->widget->setHeadersFormatted(
			variable(FetchApiVariables::kHeaders).toString());
	}

	if (hasVariable(FetchApiVariables::kSslPolicy)) {
		if (const auto policy = sslPolicyFromContext(
			variable(FetchApiVariables::kSslPolicy))) {
			d->widget->setSslPolicy(*policy);
		}
	}

	if (hasVariable(FetchApiVariables::kSslCaFile)) {
		d->widget->setCaFile(variable(FetchApiVariables::kSslCaFile).toString());
	}

	if (hasVariable(FetchApiVariables::kHandler)) {
		if (const auto handler = handlerFromContext(
			variable(FetchApiVariables::kHandler))) {
			d->widget->setHandler(*handler);
		}
	}

	d->applying = false;
}

void FetchApiDocument::writeRequestToContext() {
	if (!context() || !d->widget) {
		return;
	}

	d->applying = true;

	setVariable(FetchApiVariables::kLocation, d->widget->location());
	setVariable(FetchApiVariables::kMethod,
		Format<FetchApiMethod>::format(d->widget->method()));
	setVariable(FetchApiVariables::kBody, d->widget->bodyText());
	setVariable(FetchApiVariables::kParams, d->widget->parametersFormatted());
	setVariable(FetchApiVariables::kSslPolicy,
		Format<FetchApiSslPolicy>::format(d->widget->sslPolicy()));
	setVariable(FetchApiVariables::kHandler,
		Format<FetchApiHandlerType>::format(d->widget->handler()));

	if (d->widget->sslPolicy() == FetchApiSslPolicy::TrustCustomCa) {
		setVariable(FetchApiVariables::kSslCaFile, d->widget->caFile());
	}

	d->applying = false;
}

void FetchApiDocument::onRequestFinished(const FetchApiResult& result) {
	if (auto context = this->context()) {
		// data, content-type, headers, status-code, error, ...
		result.applyToContext(*context);
	}

	if (d->widget) {
		setToolTip(QStringLiteral("%1\n%2")
			.arg(d->widget->location())
			.arg(result.failed ? result.errorMessage() : result.contentType()));
	}

	emit requestCompleted(result);
	emit propertiesChanged();
}

void FetchApiDocument::printError(const QString& message) {
	if (auto context = this->context()) {
		if (auto* controller = context->commandController()) {
			controller->printError(QStringLiteral("fetch-api: %1").arg(message));
		}
	}
}

void FetchApiDocument::onContextAttached() {
	applyContextToWidget();
}

void FetchApiDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
	if (d->applying || !d->widget) {
		return;
	}

	if (name == QLatin1String(FetchApiVariables::kLocation)
		|| name == QLatin1String(FetchApiVariables::kUrlAlias)) {
		d->widget->setLocation(value.toString());
		return;
	}

	if (name == QLatin1String(FetchApiVariables::kMethod)) {
		if (const auto method = methodFromContext(value)) {
			d->widget->setMethod(*method);
		}
		return;
	}

	if (name == QLatin1String(FetchApiVariables::kBody)) {
		d->widget->setBodyText(value.toString());
		return;
	}

	if (name == QLatin1String(FetchApiVariables::kParams)
		|| name == QLatin1String(FetchApiVariables::kParamsAlias)) {
		d->widget->setParametersFormatted(value.toString());
		return;
	}

	if (name == QLatin1String(FetchApiVariables::kSslPolicy)) {
		if (const auto policy = sslPolicyFromContext(value)) {
			d->widget->setSslPolicy(*policy);
		}
		emit propertiesChanged();
		return;
	}

	if (name == QLatin1String(FetchApiVariables::kSslCaFile)) {
		d->widget->setCaFile(value.toString());
		emit propertiesChanged();
		return;
	}

	if (name == QLatin1String(FetchApiVariables::kHandler)) {
		if (const auto handler = handlerFromContext(value)) {
			d->widget->setHandler(*handler);
		}
		emit propertiesChanged();
		return;
	}
}

QWidget* FetchApiDocument::createPropertiesEditor(QWidget* parent) {
	auto* editor = DocumentWidget::createPropertiesEditor(parent);
	auto* form = qobject_cast<QFormLayout*>(editor->layout());
	if (!form || !d->widget) {
		return editor;
	}

	auto* requestLabel = new QLabel(editor);
	requestLabel->setWordWrap(true);
	requestLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	requestLabel->setText(QStringLiteral("%1 %2")
		.arg(Format<FetchApiMethod>::format(d->widget->method()),
			d->widget->location()));

	auto* statusLabel = new QLabel(QStringLiteral("—"), editor);
	auto* timeLabel = new QLabel(QStringLiteral("—"), editor);
	auto* sizeLabel = new QLabel(QStringLiteral("—"), editor);

	// Получатель — сама метка: связь исчезнет вместе с панелью свойств.
	connect(this, &FetchApiDocument::requestCompleted, statusLabel,
		[statusLabel, timeLabel, sizeLabel](const FetchApiResult& result) {
			statusLabel->setText(result.statusCode > 0
				? QStringLiteral("%1 %2").arg(result.statusCode).arg(result.statusText)
				: FetchApiDocument::tr("Ошибка"));
			timeLabel->setText(QStringLiteral("%1 ms").arg(result.elapsedMs));
			sizeLabel->setText(QStringLiteral("%1 B").arg(result.size));
		});

	form->insertRow(0, tr("Запрос:"), requestLabel);
	form->addRow(tr("Статус:"), statusLabel);
	form->addRow(tr("Время:"), timeLabel);
	form->addRow(tr("Размер:"), sizeLabel);
	return editor;
}

QList<DocumentPropertyGroup> FetchApiDocument::createPropertyGroups(QWidget* parent) {
	DocumentPropertyGroup request;
	request.id = kRequestGroup;
	request.title = tr("Параметры отправки");
	request.editor = d->createRequestPropertiesEditor(parent);
	return { request };
}

void FetchApiDocument::refresh() {
	applyContextToWidget();
}

bool FetchApiDocument::save() {
	// Состояние запроса живёт в контексте и не требует отдельного сохранения.
	return true;
}
