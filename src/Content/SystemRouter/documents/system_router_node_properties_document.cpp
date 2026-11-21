#include "Content/SystemRouter/documents/system_router_node_properties_document.h"
#include "Content/SystemRouter/models/system_router_mime.h"
#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/properties/property_mime.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/ui/main_window.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>
#include <optional>

namespace {

struct PropertyDescriptor {
	QString name;
	QStringList mimeTypes;
};

/// Mime-типы, определяющие редактор значения: служебные mime-типы
/// маршрутизатора (команда/параметр) не влияют на тип редактора.
QStringList valueMimeTypes(const QStringList& mimeTypes) {
	QStringList result;
	for (const auto& raw : mimeTypes) {
		const auto mime = raw.trimmed();
		if (mime.compare(QLatin1String(SystemRouterMime::kCommand), Qt::CaseInsensitive) == 0
			|| mime.compare(QLatin1String(SystemRouterMime::kCommandParameter),
				Qt::CaseInsensitive) == 0
			|| mime.compare(QLatin1String(SystemRouterMime::kShared), Qt::CaseInsensitive) == 0
			|| mime.compare(QLatin1String(SystemRouterMime::kContextVariable),
				Qt::CaseInsensitive) == 0) {
			continue;
		}
		result.append(mime);
	}
	return result;
}

QString primaryValueMimeType(const QStringList& mimeTypes) {
	return PropertyMime::primaryMimeType(valueMimeTypes(mimeTypes));
}

/// Диалог имени и набора mime-типов свойства.
/// Поддерживает произвольные mime-типы: их можно ввести вручную.
std::optional<PropertyDescriptor> requestPropertyDescriptor(QWidget* parent,
	const QString& title, const QString& name, const QStringList& mimeTypes,
	bool nameEditable) {
	QDialog dialog(parent);
	dialog.setObjectName(QStringLiteral("SystemRouterPropertyDialog"));
	dialog.setWindowTitle(title);
	dialog.resize(420, 460);

	auto* layout = new QVBoxLayout(&dialog);

	auto* nameEdit = new QLineEdit(name, &dialog);
	nameEdit->setObjectName(QStringLiteral("SystemRouterPropertyName"));
	nameEdit->setEnabled(nameEditable);

	auto* form = new QFormLayout;
	form->addRow(QCoreApplication::translate("SystemRouterNodePropertiesDocument", "Имя:"), nameEdit);
	layout->addLayout(form);

	auto* mimeGroup = new QGroupBox(
		QCoreApplication::translate("SystemRouterNodePropertiesDocument", "Mime-типы"), &dialog);
	auto* mimeLayout = new QVBoxLayout(mimeGroup);

	auto* mimeList = new QListWidget(mimeGroup);
	mimeList->setObjectName(QStringLiteral("SystemRouterPropertyMimeList"));

	// Сначала текущие mime-типы (первый — основной), затем mime-типы
	// маршрутизатора и распространённые.
	QStringList ordered = mimeTypes;
	for (const auto* routerMime : { SystemRouterMime::kCommand,
		SystemRouterMime::kCommandParameter, SystemRouterMime::kShared,
		SystemRouterMime::kContextVariable }) {
		if (!ordered.contains(QLatin1String(routerMime), Qt::CaseInsensitive)) {
			ordered.append(QLatin1String(routerMime));
		}
	}
	for (const auto& common : PropertyMime::commonMimeTypes()) {
		if (!ordered.contains(common, Qt::CaseInsensitive)) {
			ordered.append(common);
		}
	}

	QSet<QString> checked;
	for (const auto& current : mimeTypes) {
		checked.insert(current);
	}

	for (const auto& mimeType : ordered) {
		auto* item = new QListWidgetItem(mimeType, mimeList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		bool isChecked = false;
		for (const auto& current : checked) {
			if (current.compare(mimeType, Qt::CaseInsensitive) == 0) {
				isChecked = true;
				break;
			}
		}
		item->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
	}

	auto* customRow = new QHBoxLayout;
	auto* customEdit = new QLineEdit(mimeGroup);
	customEdit->setObjectName(QStringLiteral("SystemRouterPropertyCustomMime"));
	customEdit->setPlaceholderText(QCoreApplication::translate(
		"SystemRouterNodePropertiesDocument", "Другой mime-тип"));
	auto* customAdd = new QToolButton(mimeGroup);
	customAdd->setText(QStringLiteral("+"));
	customAdd->setToolTip(QCoreApplication::translate(
		"SystemRouterNodePropertiesDocument", "Добавить mime-тип"));
	customRow->addWidget(customEdit, 1);
	customRow->addWidget(customAdd);

	mimeLayout->addWidget(mimeList, 1);
	mimeLayout->addLayout(customRow);
	layout->addWidget(mimeGroup, 1);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	const auto addCustomMime = [mimeList, customEdit]() {
		const auto value = customEdit->text().trimmed();
		if (value.isEmpty()) {
			return;
		}

		for (int row = 0; row < mimeList->count(); row++) {
			auto* item = mimeList->item(row);
			if (item->text().compare(value, Qt::CaseInsensitive) == 0) {
				item->setCheckState(Qt::Checked);
				customEdit->clear();
				return;
			}
		}

		auto* item = new QListWidgetItem(value, mimeList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Checked);
		mimeList->scrollToItem(item);
		customEdit->clear();
	};

	QObject::connect(customAdd, &QToolButton::clicked, &dialog, addCustomMime);
	QObject::connect(customEdit, &QLineEdit::returnPressed, &dialog, addCustomMime);

	auto* okButton = buttons->button(QDialogButtonBox::Ok);
	const auto updateOk = [nameEdit, okButton]() {
		okButton->setEnabled(!nameEdit->text().trimmed().isEmpty());
	};
	QObject::connect(nameEdit, &QLineEdit::textChanged, &dialog,
		[updateOk](const QString&) {
			updateOk();
		});
	updateOk();

	if (nameEditable) {
		nameEdit->setFocus(Qt::OtherFocusReason);
		nameEdit->selectAll();
	}
	else {
		mimeList->setFocus(Qt::OtherFocusReason);
	}

	if (dialog.exec() != QDialog::Accepted) {
		return std::nullopt;
	}

	PropertyDescriptor descriptor;
	descriptor.name = nameEdit->text().trimmed();
	if (descriptor.name.isEmpty()) {
		return std::nullopt;
	}

	for (int row = 0; row < mimeList->count(); row++) {
		auto* item = mimeList->item(row);
		if (item->checkState() == Qt::Checked) {
			descriptor.mimeTypes.append(item->text().trimmed());
		}
	}

	return descriptor;
}

} // namespace

class SystemRouterNodePropertiesDocument::Private {
public:
	explicit Private(SystemRouterNodePropertiesDocument* parent)
		: q(parent) {
	}

	SystemRouterNodePropertiesDocument* q;
	SystemRouterService* service = nullptr;
	SystemRouterBlobService* blobService = nullptr;
	QVariant nodeId;
	LazyTreeNodePtr node;
	QSet<CommandContext*> connectedEditors;

	QLabel* header = nullptr;
	QCheckBox* expandedCheck = nullptr;
	QComboBox* selector = nullptr;
	QToolButton* addButton = nullptr;
	QToolButton* removeButton = nullptr;
	QToolButton* mimeButton = nullptr;
	PropertiesListWidget* list = nullptr;
	QLabel* status = nullptr;

	void setupUi();
	void loadNode();
	void rebuild(const QString& preferredProperty = {});
	void refreshSelector(const QString& preferredProperty = {});
	void updateActions();
	void updateExpandedCheck();
	void setStatus(const QString& message, bool error);
	void reportError(const QString& message);
	void reportWarning(const QString& message);
	bool persist();
	QString selectedProperty() const;
	PropertyData propertyData(const QString& name, const QVariant& value) const;
	void onPropertyButtonClick(const QString& name);
	QString ensureBlob(const QString& name);
	QString valueToBlobText(const QVariant& value) const;
	void openBlobEditor(const QString& name);
	void connectEditorContext(const std::shared_ptr<CommandContext>& context);
};

void SystemRouterNodePropertiesDocument::Private::setupUi() {
	auto* layout = new QVBoxLayout(q);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->setSpacing(6);

	header = new QLabel(q);
	header->setObjectName(QStringLiteral("SystemRouterNodeHeader"));
	header->setTextInteractionFlags(Qt::TextSelectableByMouse);
	header->setWordWrap(true);
	layout->addWidget(header);

	expandedCheck = new QCheckBox(SystemRouterNodePropertiesDocument::tr("Узел всегда открыт"), q);
	expandedCheck->setObjectName(QStringLiteral("SystemRouterAlwaysExpanded"));
	expandedCheck->setToolTip(SystemRouterNodePropertiesDocument::tr(
		"При загрузке дерева узел раскрывается сразу вместе с вложенными узлами"));
	layout->addWidget(expandedCheck);

	auto* tools = new QHBoxLayout;
	tools->setContentsMargins(0, 0, 0, 0);
	tools->setSpacing(4);

	auto* selectorLabel = new QLabel(SystemRouterNodePropertiesDocument::tr("Свойство:"), q);
	selector = new QComboBox(q);
	selector->setObjectName(QStringLiteral("SystemRouterPropertySelector"));
	selector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	addButton = new QToolButton(q);
	addButton->setObjectName(QStringLiteral("SystemRouterAddProperty"));
	addButton->setText(QStringLiteral("+"));
	addButton->setToolTip(SystemRouterNodePropertiesDocument::tr("Добавить свойство"));

	removeButton = new QToolButton(q);
	removeButton->setObjectName(QStringLiteral("SystemRouterRemoveProperty"));
	removeButton->setText(QStringLiteral("−"));
	removeButton->setToolTip(SystemRouterNodePropertiesDocument::tr("Удалить свойство"));

	mimeButton = new QToolButton(q);
	mimeButton->setObjectName(QStringLiteral("SystemRouterEditPropertyMime"));
	mimeButton->setText(SystemRouterNodePropertiesDocument::tr("Mime-типы…"));
	mimeButton->setToolTip(SystemRouterNodePropertiesDocument::tr("Изменить набор mime-типов"));

	tools->addWidget(selectorLabel);
	tools->addWidget(selector, 1);
	tools->addWidget(addButton);
	tools->addWidget(removeButton);
	tools->addWidget(mimeButton);
	layout->addLayout(tools);

	list = new PropertiesListWidget(q);
	list->setObjectName(QStringLiteral("SystemRouterNodePropertiesList"));
	list->setHeaderTitle(SystemRouterNodePropertiesDocument::tr("Параметры узла"));
	layout->addWidget(list, 1);

	status = new QLabel(q);
	status->setObjectName(QStringLiteral("SystemRouterNodeStatus"));
	status->setWordWrap(true);
	layout->addWidget(status);

	QObject::connect(addButton, &QToolButton::clicked, q, [this]() {
		const auto descriptor = requestPropertyDescriptor(q,
			SystemRouterNodePropertiesDocument::tr("Добавить свойство"), {}, {},
			/*nameEditable*/ true);
		if (!descriptor) {
			return;
		}

		if (!q->addProperty(descriptor->name, descriptor->mimeTypes)) {
			return;
		}

		refreshSelector(descriptor->name);
	});

	QObject::connect(removeButton, &QToolButton::clicked, q, [this]() {
		const auto name = selectedProperty();
		if (name.isEmpty()) {
			return;
		}

		q->removeProperty(name);
		refreshSelector();
	});

	QObject::connect(mimeButton, &QToolButton::clicked, q, [this]() {
		const auto name = selectedProperty();
		if (name.isEmpty() || !node) {
			return;
		}

		const auto descriptor = requestPropertyDescriptor(q,
			SystemRouterNodePropertiesDocument::tr("Mime-типы свойства"), name,
			node->propertyMimeTypes(name), /*nameEditable*/ false);
		if (!descriptor) {
			return;
		}

		q->setPropertyMimeTypes(name, descriptor->mimeTypes);
	});

	QObject::connect(selector, &QComboBox::currentIndexChanged, q,
		[this](int) {
			updateActions();
		});

	QObject::connect(list, &PropertiesListWidget::propertyChanged, q,
		[this](const QString& name, const QVariant& value) {
			q->setPropertyValue(name, value);
		});

	QObject::connect(list, &PropertiesListWidget::propertyButtonClick, q,
		[this](const QString& name) {
			onPropertyButtonClick(name);
		});

	QObject::connect(expandedCheck, &QCheckBox::toggled, q,
		[this](bool checked) {
			// Флажок отражает значение из базы: при ошибке сохранения
			// возвращаем его в прежнее состояние.
			if (!q->setAlwaysExpanded(checked)) {
				updateExpandedCheck();
			}
		});
}

void SystemRouterNodePropertiesDocument::Private::loadNode() {
	node = nullptr;
	if (service && nodeId.isValid() && !nodeId.isNull()) {
		node = service->item(nodeId);
	}

	rebuild();
	if (service && !node && nodeId.isValid() && !nodeId.isNull()) {
		setStatus(SystemRouterNodePropertiesDocument::tr("Узел не найден"), true);
	}
}

void SystemRouterNodePropertiesDocument::Private::rebuild(const QString& preferredProperty) {
	if (!q) {
		return;
	}

	if (node) {
		header->setText(SystemRouterNodePropertiesDocument::tr("Узел: %1\nid: %2")
			.arg(node->name(), node->id().toString()));
	}
	else {
		header->setText(SystemRouterNodePropertiesDocument::tr("Узел не выбран"));
	}

	if (list) {
		QList<PropertyData> properties;
		if (node) {
			const auto values = node->properties();
			for (auto it = values.constBegin(); it != values.constEnd(); it++) {
				properties.append(propertyData(it.key(), it.value()));
			}
		}

		list->setProperties(properties);
	}

	refreshSelector(preferredProperty);
	updateActions();
}

void SystemRouterNodePropertiesDocument::Private::updateExpandedCheck() {
	if (!expandedCheck) {
		return;
	}

	const QSignalBlocker blocker(expandedCheck);
	expandedCheck->setChecked(node && node->expanded());
	expandedCheck->setEnabled(node != nullptr);
}

PropertyData SystemRouterNodePropertiesDocument::Private::propertyData(const QString& name,
	const QVariant& value) const {
	const auto mimeTypes = node ? node->propertyMimeTypes(name) : QStringList();
	const auto primary = primaryValueMimeType(mimeTypes);

	PropertyData data;
	data.id = name;
	data.label = name;
	data.tooltip = mimeTypes.isEmpty()
		? SystemRouterNodePropertiesDocument::tr("Mime-типы не заданы")
		: SystemRouterNodePropertiesDocument::tr("Mime-типы: %1").arg(mimeTypes.join(QStringLiteral(", ")));

	if (SystemRouterMime::isCodeEditorProperty(mimeTypes)) {
		data.type = PropertyType::Button;
		data.tooltip += SystemRouterNodePropertiesDocument::tr("\nОткрыть в редакторе кода");
		const auto command = node ? node->properties().value(name).toString() : QString();
		if (!command.isEmpty()) {
			data.tooltip += SystemRouterNodePropertiesDocument::tr("\nКоманда: %1").arg(command);
		}
		return data;
	}

	data.type = PropertyMime::editorType(primary);
	data.value = PropertyMime::normalizeValue(value, primary);
	data.defaultValue = PropertyMime::defaultValue(primary);

	if (data.type == PropertyType::Number) {
		PropertyMime::applyNumberLimits(data, primary);
	}
	else if (data.type == PropertyType::PathFile) {
		data.fileFilter = SystemRouterNodePropertiesDocument::tr("Все файлы (*.*)");
	}

	return data;
}

void SystemRouterNodePropertiesDocument::Private::reportError(const QString& message) {
	if (auto* controller = q->context() ? q->context()->commandController() : nullptr) {
		controller->printError(message);
	}

	setStatus(message, true);
	emit q->errorOccurred(message);
}

void SystemRouterNodePropertiesDocument::Private::reportWarning(const QString& message) {
	if (auto* controller = q->context() ? q->context()->commandController() : nullptr) {
		controller->printWarning(message);
	}

	setStatus(message, false);
}

QString SystemRouterNodePropertiesDocument::Private::valueToBlobText(const QVariant& value) const {
	const auto type = value.metaType().id();
	if (type == QMetaType::QVariantMap || type == QMetaType::QVariantList) {
		return QString::fromUtf8(
			QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact));
	}

	return value.toString();
}

QString SystemRouterNodePropertiesDocument::Private::ensureBlob(const QString& name) {
	if (!node) {
		return {};
	}

	const auto mimeTypes = node->propertyMimeTypes(name);
	const bool command = std::any_of(mimeTypes.cbegin(), mimeTypes.cend(),
		[](const QString& mime) {
			return mime.trimmed().compare(QLatin1String(SystemRouterMime::kCommand),
				Qt::CaseInsensitive) == 0;
		});

	if (command) {
		// Для команды значение свойства — GUID, он же идентификатор блока
		// со скриптом в data_blobs.
		const auto raw = node->properties().value(name).toString().trimmed();
		if (raw.isEmpty()) {
			reportError(SystemRouterNodePropertiesDocument::tr("Ссылка на команду не задана"));
			return {};
		}

		const auto uuid = QUuid::fromString(raw);
		const auto blobId = uuid.isNull() ? raw : uuid.toString(QUuid::WithoutBraces);
		if (!blobService) {
			reportError(SystemRouterNodePropertiesDocument::tr("Сервис блоков данных недоступен"));
			return {};
		}

		if (!blobService->blob(blobId).has_value()
			&& !blobService->saveData(blobId, QString())) {
			reportError(blobService->lastError().isEmpty()
				? SystemRouterNodePropertiesDocument::tr("Не удалось создать блок команды")
				: blobService->lastError());
			return {};
		}

		return blobId;
	}

	auto blobId = node->propertyValueBlobId(name).trimmed();
	if (!blobId.isEmpty()) {
		return blobId;
	}

	if (!blobService) {
		reportError(SystemRouterNodePropertiesDocument::tr("Сервис блоков данных недоступен"));
		return {};
	}

	blobId = blobService->createData(valueToBlobText(node->properties().value(name)));
	if (blobId.isEmpty()) {
		reportError(blobService->lastError().isEmpty()
			? SystemRouterNodePropertiesDocument::tr("Не удалось создать блок данных")
			: blobService->lastError());
		return {};
	}

	node->setPropertyValueBlobId(name, blobId);
	if (!persist()) {
		blobService->removeBlob(blobId);
		node->setPropertyValueBlobId(name, {});
		return {};
	}

	return blobId;
}

void SystemRouterNodePropertiesDocument::Private::connectEditorContext(
	const std::shared_ptr<CommandContext>& context) {
	if (!context || connectedEditors.contains(context.get())) {
		return;
	}

	connectedEditors.insert(context.get());
	QPointer<CommandContext> editorContext(context.get());
	QObject::connect(context.get(), &CommandContext::eventFired, q,
		[this, editorContext](const QString& eventName) {
			if (!editorContext || eventName != CommandContext::documentSaveCompleteEvent()) {
				return;
			}

			auto editor = editorContext->shared_from_this();
			if (!editor) {
				return;
			}

			auto* controller = editor->commandController();
			auto* processor = controller ? controller->commandProcessor() : nullptr;
			if (!processor) {
				reportError(SystemRouterNodePropertiesDocument::tr(
					"Процессор команд недоступен"));
				return;
			}

			processor->execute(QStringLiteral("system-router-save-blob"), editor);
			q->refresh();
		});
	QObject::connect(context.get(), &QObject::destroyed, q, [this, raw = context.get()] {
		connectedEditors.remove(raw);
	});
}

void SystemRouterNodePropertiesDocument::Private::openBlobEditor(const QString& name) {
	if (!node || !q->context()) {
		return;
	}

	const auto mimeTypes = node->propertyMimeTypes(name);
	if (!SystemRouterMime::isCodeEditorProperty(mimeTypes)) {
		return;
	}

	const auto blobId = ensureBlob(name);
	if (blobId.isEmpty()) {
		return;
	}

	if (!blobService) {
		reportError(SystemRouterNodePropertiesDocument::tr("Сервис блоков данных недоступен"));
		return;
	}

	const auto blob = blobService->blob(blobId);
	if (!blob.has_value()) {
		reportError(SystemRouterNodePropertiesDocument::tr("Блок данных не найден: %1")
			.arg(blobId));
		return;
	}

	auto* controller = q->context()->commandController();
	auto* processor = controller ? controller->commandProcessor() : nullptr;
	auto* window = controller ? controller->mainWindow() : nullptr;
	if (!processor || !window) {
		reportError(SystemRouterNodePropertiesDocument::tr(
			"Не удалось открыть редактор: контроллер или окно недоступны"));
		return;
	}

	const auto editorId = QStringLiteral("edit-value-blob-%1").arg(blobId);
	auto commandContext = q->context()->createScopedContext(
		QStringLiteral("system-router-open-blob"));
	commandContext->setVariable(QStringLiteral("data"), blob->data);
	commandContext->setVariableMimeType(QStringLiteral("data"),
		QString::fromLatin1(PropertyMimeType::kShared));
	commandContext->setVariable(QStringLiteral("blobId"), blobId);
	commandContext->setVariableMimeType(QStringLiteral("blobId"),
		QString::fromLatin1(PropertyMimeType::kShared));

	const auto command = QStringLiteral("documents action=create, target=code-editor, id=%1")
		.arg(editorId);
	if (!processor->execute(command, commandContext)) {
		reportError(SystemRouterNodePropertiesDocument::tr(
			"Не удалось открыть редактор кода"));
		return;
	}

	auto* editor = window->document(editorId);
	if (!editor || !editor->context()) {
		reportError(SystemRouterNodePropertiesDocument::tr(
			"Документ редактора кода не создан"));
		return;
	}

	auto editorContext = editor->context();
	editorContext->setVariable(QStringLiteral("data"), blob->data);
	editorContext->setVariable(QStringLiteral("blobId"), blobId);
	editorContext->setVariableMimeType(QStringLiteral("data"),
		QString::fromLatin1(PropertyMimeType::kShared));
	editorContext->setVariableMimeType(QStringLiteral("blobId"),
		QString::fromLatin1(PropertyMimeType::kShared));
	connectEditorContext(editorContext);
}

void SystemRouterNodePropertiesDocument::Private::onPropertyButtonClick(const QString& name) {
	openBlobEditor(name);
}

void SystemRouterNodePropertiesDocument::Private::refreshSelector(const QString& preferredProperty) {
	if (!selector) {
		return;
	}

	const auto current = preferredProperty.isEmpty()
		? selector->currentData().toString()
		: preferredProperty;
	const QSignalBlocker blocker(selector);
	selector->clear();

	if (node) {
		const auto names = node->properties().keys();
		for (const auto& name : names) {
			selector->addItem(name, name);
		}
	}

	int index = selector->findData(current);
	if (index < 0 && selector->count() > 0) {
		index = 0;
	}
	selector->setCurrentIndex(index);
	updateActions();
}

void SystemRouterNodePropertiesDocument::Private::updateActions() {
	const bool hasNode = node != nullptr;
	const bool hasProperty = hasNode && !selectedProperty().isEmpty();
	addButton->setEnabled(hasNode);
	removeButton->setEnabled(hasProperty);
	mimeButton->setEnabled(hasProperty);
	updateExpandedCheck();
}

QString SystemRouterNodePropertiesDocument::Private::selectedProperty() const {
	return selector ? selector->currentData().toString() : QString();
}

void SystemRouterNodePropertiesDocument::Private::setStatus(const QString& message, bool error) {
	if (!status) {
		return;
	}

	status->setText(message);
	status->setProperty("state", error ? QStringLiteral("error") : QStringLiteral("success"));
	status->style()->unpolish(status);
	status->style()->polish(status);
}

bool SystemRouterNodePropertiesDocument::Private::persist() {
	if (!service || !node) {
		return false;
	}

	if (!service->updateItem(node)) {
		const auto message = service->lastError().isEmpty()
			? SystemRouterNodePropertiesDocument::tr("Не удалось сохранить параметры узла")
			: service->lastError();
		reportError(message);
		return false;
	}

	setStatus(SystemRouterNodePropertiesDocument::tr("Сохранено"), false);
	return true;
}

SystemRouterNodePropertiesDocument::SystemRouterNodePropertiesDocument(SystemRouterService* service,
	SystemRouterBlobService* blobService, const QVariant& nodeId, QWidget* parent)
	: DocumentWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->service = service;
	d->blobService = blobService;
	d->nodeId = nodeId;

	setObjectName(QStringLiteral("SystemRouterNodePropertiesDocument"));
	setGlyph(QStringLiteral("🛠"));
	setTitle(tr("Параметры узла"));

	d->setupUi();
	d->loadNode();
}

SystemRouterNodePropertiesDocument::~SystemRouterNodePropertiesDocument() = default;

QVariant SystemRouterNodePropertiesDocument::nodeId() const {
	return d->nodeId;
}

LazyTreeNodePtr SystemRouterNodePropertiesDocument::node() const {
	return d->node;
}

bool SystemRouterNodePropertiesDocument::alwaysExpanded() const {
	return d->node && d->node->expanded();
}

bool SystemRouterNodePropertiesDocument::setAlwaysExpanded(bool value) {
	if (!d->node || !d->service) {
		return false;
	}

	if (!d->service->setAlwaysExpanded(d->node->id(), value)) {
		const auto message = d->service->lastError().isEmpty()
			? tr("Не удалось сохранить признак «узел всегда открыт»")
			: d->service->lastError();
		d->reportError(message);
		return false;
	}

	d->node->setExpanded(value);
	d->updateExpandedCheck();
	d->setStatus(tr("Сохранено"), false);
	return true;
}

bool SystemRouterNodePropertiesDocument::addProperty(const QString& name,
	const QStringList& mimeTypes, const QVariant& value) {
	if (!d->node || !d->service) {
		return false;
	}

	const auto trimmed = name.trimmed();
	if (trimmed.isEmpty() || d->node->properties().contains(trimmed)) {
		d->setStatus(tr("Свойство уже существует или имя пустое"), true);
		return false;
	}

	const auto primary = primaryValueMimeType(mimeTypes);
	auto actualValue = value.isValid()
		? PropertyMime::normalizeValue(value, primary)
		: PropertyMime::defaultValue(primary);

	const bool commandProperty = std::any_of(mimeTypes.cbegin(), mimeTypes.cend(),
		[](const QString& mime) {
			return mime.trimmed().compare(QLatin1String(SystemRouterMime::kCommand),
				Qt::CaseInsensitive) == 0;
		});
	if (commandProperty && actualValue.toString().trimmed().isEmpty()) {
		actualValue = QUuid::createUuid().toString(QUuid::WithoutBraces);
	}

	d->node->setProperty(trimmed, actualValue);
	d->node->setPropertyMimeTypes(trimmed, mimeTypes);

	if (!d->persist()) {
		d->node->removeProperty(trimmed);
		d->rebuild();
		return false;
	}

	d->rebuild(trimmed);
	return true;
}

bool SystemRouterNodePropertiesDocument::removeProperty(const QString& name) {
	if (!d->node || !d->service) {
		return false;
	}

	const auto values = d->node->properties();
	const auto it = values.constFind(name);
	if (it == values.constEnd()) {
		return false;
	}

	const auto oldValue = it.value();
	const auto oldMimeTypes = d->node->propertyMimeTypes(name);

	if (!d->node->removeProperty(name)) {
		return false;
	}

	if (!d->persist()) {
		d->node->setProperty(name, oldValue);
		d->node->setPropertyMimeTypes(name, oldMimeTypes);
		d->rebuild(name);
		return false;
	}

	d->rebuild();
	return true;
}

bool SystemRouterNodePropertiesDocument::setPropertyValue(const QString& name,
	const QVariant& value) {
	if (!d->node || !d->service) {
		return false;
	}

	const auto values = d->node->properties();
	const auto it = values.constFind(name);
	if (it == values.constEnd()) {
		return false;
	}

	const auto oldValue = it.value();
	if (!d->node->setProperty(name, value)) {
		return false;
	}

	if (!d->persist()) {
		d->node->setProperty(name, oldValue);
		d->rebuild(name);
		return false;
	}

	return true;
}

bool SystemRouterNodePropertiesDocument::setPropertyMimeTypes(const QString& name,
	const QStringList& mimeTypes) {
	if (!d->node || !d->service || !d->node->properties().contains(name)) {
		return false;
	}

	const auto oldMimeTypes = d->node->propertyMimeTypes(name);
	const auto oldValue = d->node->properties().value(name);
	if (!d->node->setPropertyMimeTypes(name, mimeTypes)) {
		return false;
	}

	const bool commandProperty = std::any_of(mimeTypes.cbegin(), mimeTypes.cend(),
		[](const QString& mime) {
			return mime.trimmed().compare(QLatin1String(SystemRouterMime::kCommand),
				Qt::CaseInsensitive) == 0;
		});
	if (commandProperty && QUuid::fromString(oldValue.toString().trimmed()).isNull()) {
		d->node->setProperty(name, QUuid::createUuid().toString(QUuid::WithoutBraces));
	}

	if (!d->persist()) {
		d->node->setPropertyMimeTypes(name, oldMimeTypes);
		d->node->setProperty(name, oldValue);
		d->rebuild(name);
		return false;
	}

	// Редактор значения зависит от основного mime-типа: перестраиваем список.
	d->rebuild(name);
	return true;
}

void SystemRouterNodePropertiesDocument::refresh() {
	if (!d->service) {
		return;
	}

	d->loadNode();
}

bool SystemRouterNodePropertiesDocument::save() {
	// Изменения сохраняются сразу при редактировании.
	return true;
}

void SystemRouterNodePropertiesDocument::onContextAttached() {
	if (!d->service) {
		return;
	}

	if (!d->node) {
		const auto id = variable(QStringLiteral("nodeId"));
		if (id.isValid() && !id.isNull()) {
			d->nodeId = id;
			d->loadNode();
		}
	}
	else if (!hasVariable(QStringLiteral("nodeId"))) {
		setVariable(QStringLiteral("nodeId"), d->nodeId);
	}

	if (title().isEmpty() && d->node) {
		setTitle(tr("Параметры: %1").arg(d->node->name()));
	}
}
