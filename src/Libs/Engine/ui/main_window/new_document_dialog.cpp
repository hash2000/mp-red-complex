#include "Libs/Engine/ui/main_window/new_document_dialog.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/themes/document_tab_theme.h"
#include "Libs/Resources/resources.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPair>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>
#include <optional>

namespace {
/// Роль элемента списка: xtype зарегистрированного типа.
constexpr int kTypeXtypeRole = Qt::UserRole + 1;

/// Редактор значения переменной по mime-типу.
PropertyType propertyTypeForMime(const QString& mimeType) {
	return PropertyMime::editorType(mimeType);
}

bool isIntegerMime(const QString& mimeType) {
	return PropertyMime::isInteger(mimeType);
}

/// Значение переменной по умолчанию для mime-типа.
QVariant defaultValueForMime(const QString& mimeType) {
	return PropertyMime::defaultValue(mimeType);
}

/// Приводит значение из контекста к типу, который ожидает редактор.
QVariant normalizeValue(const QVariant& value, const QString& mimeType) {
	return PropertyMime::normalizeValue(value, mimeType);
}

/// Ограничения редактора числа: целые и дробные значения.
void applyNumberLimits(PropertyData& property, const QString& mimeType) {
	PropertyMime::applyNumberLimits(property, mimeType);
}

/// Тема приложения: из неё берутся стандартные глифы вкладок по типу.
DocumentTabTheme loadTheme(const std::shared_ptr<CommandContext>& context) {
	auto* controller = context ? context->commandController() : nullptr;
	auto* resources = controller ? controller->resources() : nullptr;
	if (!resources) {
		return DocumentTabTheme();
	}

	const auto themeDir = resources->Directories.get(QStringLiteral("path.theme"));
	if (!themeDir) {
		return DocumentTabTheme();
	}

	return DocumentTabTheme::loadFromThemeDirectory(themeDir->absolutePath());
}
} // namespace

class NewDocumentDialog::Private {
public:
	/// Параметр документа: описание переменной и признак обязательности.
	struct VariableEntry {
		DocumentVariable variable;
		bool required = false;
	};

	NewDocumentDialog* q = nullptr;
	WidgetRegistry* registry = nullptr;
	std::shared_ptr<CommandContext> context;
	/// Оформление вкладок из темы: стандартный глиф выбранного типа.
	DocumentTabTheme theme;

	QLineEdit* searchEdit = nullptr;
	QListWidget* typeList = nullptr;

	QLabel* classNameValue = nullptr;
	QLabel* xtypeValue = nullptr;
	QLabel* singletoneValue = nullptr;

	QLineEdit* nameEdit = nullptr;
	QToolButton* nameRandomButton = nullptr;
	QLabel* nameHint = nullptr;
	QLineEdit* glyphEdit = nullptr;
	QLineEdit* titleEdit = nullptr;

	QToolButton* addVariableButton = nullptr;
	QToolButton* removeVariableButton = nullptr;
	QComboBox* variableSelector = nullptr;
	PropertiesListWidget* variablesList = nullptr;
	QDialogButtonBox* buttons = nullptr;

	/// Параметры выбранного типа: обязательные из метаданных и добавленные вручную.
	QList<VariableEntry> variableEntries;
	/// Текущие значения параметров.
	QVariantMap documentVariables;

	void setupUi(NewDocumentDialog* q);
	void fillTypes();
	void fillDocumentVariables();
	void addDocumentVariable();
	void removeSelectedDocumentVariable();
	void refreshVariableSelector(const QString& preferred = QString());
	void updateRemoveVariableState();
	bool hasDocumentVariable(const QString& name) const;
	PropertyData propertyDataFor(const DocumentVariable& variable, const QVariant& value) const;
	std::optional<DocumentVariable> requestDocumentVariable();
	void applyFilter(const QString& text);
	void updateDetails();
	void updateAcceptState();
	void randomizeName();
	void selectType(QListWidgetItem* item);
	void acceptDialog(NewDocumentDialog* q);
	QVariantMap variables() const;
	std::optional<WidgetTypeHandler> currentType() const;
};

void NewDocumentDialog::Private::setupUi(NewDocumentDialog* q) {
	q->setObjectName(QStringLiteral("NewDocumentDialog"));
	q->setWindowTitle(NewDocumentDialog::tr("Новый документ"));
	q->resize(920, 560);
	q->setSizeGripEnabled(true);

	auto* root = new QVBoxLayout(q);
	root->setContentsMargins(10, 10, 10, 10);
	root->setSpacing(8);

	auto* splitter = new QSplitter(Qt::Horizontal, q);
	splitter->setChildrenCollapsible(false);

	// -------------------------------------------------------- левая часть
	auto* left = new QWidget(splitter);
	auto* leftLayout = new QVBoxLayout(left);
	leftLayout->setContentsMargins(0, 0, 0, 0);
	leftLayout->setSpacing(6);

	searchEdit = new QLineEdit(left);
	searchEdit->setObjectName(QStringLiteral("NewDocumentDialogSearch"));
	searchEdit->setPlaceholderText(
		NewDocumentDialog::tr("Поиск по xtype или имени класса..."));
	searchEdit->setClearButtonEnabled(true);

	typeList = new QListWidget(left);
	typeList->setObjectName(QStringLiteral("NewDocumentDialogTypes"));
	typeList->setSelectionMode(QAbstractItemView::SingleSelection);
	typeList->setAlternatingRowColors(true);
	typeList->setUniformItemSizes(true);

	leftLayout->addWidget(searchEdit);
	leftLayout->addWidget(typeList, 1);

	// -------------------------------------------------------- правая часть
	auto* right = new QWidget(splitter);
	auto* rightLayout = new QVBoxLayout(right);
	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->setSpacing(8);

	// Сведения о выбранном типе.
	auto* infoGroup = new QGroupBox(NewDocumentDialog::tr("Тип документа"), right);
	auto* infoForm = new QFormLayout(infoGroup);
	infoForm->setLabelAlignment(Qt::AlignRight);

	classNameValue = new QLabel(infoGroup);
	classNameValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
	classNameValue->setWordWrap(true);
	xtypeValue = new QLabel(infoGroup);
	xtypeValue->setTextInteractionFlags(Qt::TextSelectableByMouse);
	singletoneValue = new QLabel(infoGroup);

	infoForm->addRow(NewDocumentDialog::tr("Класс:"), classNameValue);
	infoForm->addRow(NewDocumentDialog::tr("xtype:"), xtypeValue);
	infoForm->addRow(NewDocumentDialog::tr("Синглтон:"), singletoneValue);

	// Параметры создания.
	auto* paramsGroup = new QGroupBox(NewDocumentDialog::tr("Параметры создания"), right);
	auto* paramsForm = new QFormLayout(paramsGroup);
	paramsForm->setLabelAlignment(Qt::AlignRight);

	auto* nameRow = new QWidget(paramsGroup);
	auto* nameLayout = new QHBoxLayout(nameRow);
	nameLayout->setContentsMargins(0, 0, 0, 0);
	nameLayout->setSpacing(6);

	nameEdit = new QLineEdit(nameRow);
	nameEdit->setObjectName(QStringLiteral("NewDocumentDialogName"));
	nameRandomButton = new QToolButton(nameRow);
	nameRandomButton->setObjectName(QStringLiteral("NewDocumentDialogRandomName"));
	nameRandomButton->setText(NewDocumentDialog::tr("GUID"));
	nameRandomButton->setToolTip(NewDocumentDialog::tr("Случайное имя (GUID)"));
	nameLayout->addWidget(nameEdit, 1);
	nameLayout->addWidget(nameRandomButton);

	nameHint = new QLabel(paramsGroup);
	nameHint->setWordWrap(true);

	glyphEdit = new QLineEdit(paramsGroup);
	glyphEdit->setObjectName(QStringLiteral("NewDocumentDialogGlyph"));
	glyphEdit->setMaxLength(4);
	titleEdit = new QLineEdit(paramsGroup);
	titleEdit->setObjectName(QStringLiteral("NewDocumentDialogTitle"));

	paramsForm->addRow(NewDocumentDialog::tr("Имя:"), nameRow);
	paramsForm->addRow(QString(), nameHint);
	paramsForm->addRow(NewDocumentDialog::tr("Значок:"), glyphEdit);
	paramsForm->addRow(NewDocumentDialog::tr("Заголовок:"), titleEdit);

	// Параметры документа: обязательные переменные типа и добавленные вручную.
	// Редактор значения выбирается по mime-типу переменной.
	auto* variablesTools = new QHBoxLayout;
	variablesTools->setContentsMargins(0, 0, 0, 0);
	variablesTools->setSpacing(4);

	addVariableButton = new QToolButton(right);
	addVariableButton->setObjectName(QStringLiteral("NewDocumentDialogAddVariable"));
	addVariableButton->setText(QStringLiteral("+"));
	addVariableButton->setToolTip(NewDocumentDialog::tr("Добавить параметр"));
	removeVariableButton = new QToolButton(right);
	removeVariableButton->setObjectName(QStringLiteral("NewDocumentDialogRemoveVariable"));
	removeVariableButton->setText(QStringLiteral("−"));
	removeVariableButton->setToolTip(NewDocumentDialog::tr("Удалить выбранный параметр"));
	removeVariableButton->setEnabled(false);

	variableSelector = new QComboBox(right);
	variableSelector->setObjectName(QStringLiteral("NewDocumentDialogVariableSelector"));
	variableSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	variablesTools->addWidget(addVariableButton);
	variablesTools->addWidget(removeVariableButton);
	variablesTools->addWidget(variableSelector, 1);

	variablesList = new PropertiesListWidget(right);
	variablesList->setObjectName(QStringLiteral("NewDocumentDialogVariables"));
	variablesList->setHeaderTitle(NewDocumentDialog::tr("Переменные документа"));

	rightLayout->addWidget(infoGroup);
	rightLayout->addWidget(paramsGroup);
	rightLayout->addLayout(variablesTools);
	rightLayout->addWidget(variablesList, 1);

	splitter->addWidget(left);
	splitter->addWidget(right);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);
	splitter->setSizes({ 340, 560 });

	root->addWidget(splitter, 1);

	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q);
	root->addWidget(buttons);

	QObject::connect(searchEdit, &QLineEdit::textChanged, q, [this](const QString& text) {
		applyFilter(text);
	});
	QObject::connect(typeList, &QListWidget::currentItemChanged, q,
		[this](QListWidgetItem* current, QListWidgetItem*) {
			selectType(current);
		});
	QObject::connect(nameEdit, &QLineEdit::textChanged, q, [this](const QString&) {
		updateAcceptState();
	});
	QObject::connect(nameRandomButton, &QToolButton::clicked, q, [this] {
		randomizeName();
	});
	QObject::connect(addVariableButton, &QToolButton::clicked, q, [this] {
		addDocumentVariable();
	});
	QObject::connect(removeVariableButton, &QToolButton::clicked, q, [this] {
		removeSelectedDocumentVariable();
	});
	QObject::connect(variableSelector, &QComboBox::currentIndexChanged, q, [this](int) {
		updateRemoveVariableState();
	});
	QObject::connect(variablesList, &PropertiesListWidget::propertyChanged, q,
		[this](const QString& id, const QVariant& value) {
			documentVariables.insert(id, value);
		});
	QObject::connect(buttons, &QDialogButtonBox::accepted, q, [this, q] {
		acceptDialog(q);
	});
	QObject::connect(buttons, &QDialogButtonBox::rejected, q, &QDialog::reject);

	fillTypes();
	updateDetails();
}

void NewDocumentDialog::Private::fillTypes() {
	typeList->clear();
	if (!registry) {
		return;
	}

	for (const auto& type : registry->types()) {
		auto* item = new QListWidgetItem(
			QStringLiteral("(%1) %2").arg(type.xtype, type.className), typeList);
		item->setData(kTypeXtypeRole, type.xtype);
		item->setToolTip(type.title);
	}

	if (typeList->count() > 0) {
		typeList->setCurrentRow(0);
	}
}

void NewDocumentDialog::Private::fillDocumentVariables() {
	variableEntries.clear();
	documentVariables.clear();

	QList<PropertyData> properties;
	const auto type = currentType();
	const auto* metadata = (type && registry)
		? registry->documentMetadata(type->xtype) : nullptr;

	if (metadata) {
		for (auto variable : metadata->variables()) {
			if (variable.name.isEmpty() || hasDocumentVariable(variable.name)) {
				continue;
			}
			if (variable.mimeType.trimmed().isEmpty()) {
				variable.mimeType = QString::fromLatin1(DocumentVariableMime::kText);
			}

			variableEntries.append(VariableEntry{ variable, /*required*/ true });
			const auto contextValue = context ? context->variable(variable.name) : QVariant();
			const auto value = normalizeValue(contextValue, variable.mimeType);
			documentVariables.insert(variable.name, value);
			properties.append(propertyDataFor(variable, value));
		}
	}

	variablesList->setProperties(properties);
	refreshVariableSelector();
}

bool NewDocumentDialog::Private::hasDocumentVariable(const QString& name) const {
	return std::any_of(variableEntries.cbegin(), variableEntries.cend(),
		[&name](const VariableEntry& entry) {
			return entry.variable.name == name;
		});
}

PropertyData NewDocumentDialog::Private::propertyDataFor(const DocumentVariable& variable,
	const QVariant& value) const {
	const auto mimeType = variable.mimeType.trimmed().isEmpty()
		? QString::fromLatin1(DocumentVariableMime::kText)
		: variable.mimeType.trimmed();
	const auto propertyType = propertyTypeForMime(mimeType);

	PropertyData property;
	property.id = variable.name;
	property.label = variable.name;
	property.type = propertyType;
	property.value = value;
	property.defaultValue = defaultValueForMime(mimeType);
	property.tooltip = NewDocumentDialog::tr("Mime-тип: %1").arg(mimeType);

	if (propertyType == PropertyType::PathFile) {
		property.fileFilter = NewDocumentDialog::tr("Все файлы (*.*)");
	}
	else if (propertyType == PropertyType::Number) {
		applyNumberLimits(property, mimeType);
	}

	return property;
}

void NewDocumentDialog::Private::refreshVariableSelector(const QString& preferred) {
	if (!variableSelector) {
		return;
	}

	const auto current = preferred.isEmpty()
		? variableSelector->currentData().toString()
		: preferred;
	const QSignalBlocker blocker(variableSelector);
	variableSelector->clear();
	for (const auto& entry : variableEntries) {
		variableSelector->addItem(entry.variable.name, entry.variable.name);
	}

	int index = variableSelector->findData(current);
	if (index < 0 && variableSelector->count() > 0) {
		index = 0;
	}
	variableSelector->setCurrentIndex(index);
	updateRemoveVariableState();
}

void NewDocumentDialog::Private::updateRemoveVariableState() {
	if (!removeVariableButton || !variableSelector) {
		return;
	}

	const auto name = variableSelector->currentData().toString();
	bool removable = false;
	for (const auto& entry : variableEntries) {
		if (entry.variable.name == name) {
			removable = !entry.required;
			break;
		}
	}

	removeVariableButton->setEnabled(removable);
}

void NewDocumentDialog::Private::addDocumentVariable() {
	const auto variable = requestDocumentVariable();
	if (!variable) {
		return;
	}

	variableEntries.append(VariableEntry{ *variable, /*required*/ false });
	const auto contextValue = context ? context->variable(variable->name) : QVariant();
	const auto value = normalizeValue(contextValue, variable->mimeType);
	documentVariables.insert(variable->name, value);
	variablesList->addProperty(propertyDataFor(*variable, value));
	refreshVariableSelector(variable->name);
}

void NewDocumentDialog::Private::removeSelectedDocumentVariable() {
	if (!variableSelector) {
		return;
	}

	const auto name = variableSelector->currentData().toString();
	if (name.isEmpty()) {
		return;
	}

	for (int index = 0; index < variableEntries.size(); index++) {
		if (variableEntries.at(index).variable.name != name) {
			continue;
		}
		if (variableEntries.at(index).required) {
			return;
		}
		variableEntries.removeAt(index);
		break;
	}

	documentVariables.remove(name);
	variablesList->removeProperty(name);
	refreshVariableSelector();
}

std::optional<DocumentVariable> NewDocumentDialog::Private::requestDocumentVariable() {
	QDialog dialog(q);
	dialog.setObjectName(QStringLiteral("NewDocumentVariableDialog"));
	dialog.setWindowTitle(NewDocumentDialog::tr("Добавить параметр"));

	auto* layout = new QVBoxLayout(&dialog);
	auto* form = new QFormLayout;

	auto* nameEdit = new QLineEdit(&dialog);
	nameEdit->setObjectName(QStringLiteral("NewDocumentVariableName"));
	auto* mimeCombo = new QComboBox(&dialog);
	mimeCombo->setObjectName(QStringLiteral("NewDocumentVariableMime"));

	// Пустой mime-тип трактуется как обычный текст; первый пункт — по умолчанию.
	const QList<QPair<QString, QString>> mimeChoices = {
		{ QString::fromLatin1(DocumentVariableMime::kText),
			NewDocumentDialog::tr("Обычный текст") },
		{ QString::fromLatin1(DocumentVariableMime::kMultiline),
			NewDocumentDialog::tr("Многострочный текст") },
		{ QString::fromLatin1(DocumentVariableMime::kFilePath),
			NewDocumentDialog::tr("Путь к файлу") },
		{ QString::fromLatin1(DocumentVariableMime::kDirectoryPath),
			NewDocumentDialog::tr("Путь к каталогу") },
		{ QString::fromLatin1(DocumentVariableMime::kBoolean),
			NewDocumentDialog::tr("Логическое значение") },
		{ QString::fromLatin1(DocumentVariableMime::kInteger),
			NewDocumentDialog::tr("Целое число") },
		{ QString::fromLatin1(DocumentVariableMime::kNumber),
			NewDocumentDialog::tr("Число") },
		{ QString::fromLatin1(DocumentVariableMime::kColor),
			NewDocumentDialog::tr("Цвет") }
	};
	for (const auto& choice : mimeChoices) {
		mimeCombo->addItem(choice.second, choice.first);
	}

	form->addRow(NewDocumentDialog::tr("Имя:"), nameEdit);
	form->addRow(NewDocumentDialog::tr("Mime-тип:"), mimeCombo);
	layout->addLayout(form);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	layout->addWidget(buttons);
	QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

	auto* ok = buttons->button(QDialogButtonBox::Ok);
	const auto updateOk = [this, nameEdit, ok]() {
		const auto name = nameEdit->text().trimmed();
		ok->setEnabled(!name.isEmpty() && !hasDocumentVariable(name));
	};
	QObject::connect(nameEdit, &QLineEdit::textChanged, &dialog,
		[updateOk](const QString&) {
			updateOk();
		});
	updateOk();
	nameEdit->setFocus(Qt::OtherFocusReason);
	dialog.adjustSize();

	if (dialog.exec() != QDialog::Accepted) {
		return std::nullopt;
	}

	const auto name = nameEdit->text().trimmed();
	if (name.isEmpty() || hasDocumentVariable(name)) {
		return std::nullopt;
	}

	auto mimeType = mimeCombo->currentData().toString();
	if (mimeType.isEmpty()) {
		mimeType = QString::fromLatin1(DocumentVariableMime::kText);
	}

	return DocumentVariable{ name, mimeType };
}

void NewDocumentDialog::Private::applyFilter(const QString& text) {
	const auto query = text.trimmed();
	auto* current = typeList->currentItem();
	bool currentVisible = false;
	QListWidgetItem* firstVisible = nullptr;

	for (int row = 0; row < typeList->count(); row++) {
		auto* item = typeList->item(row);
		const auto xtype = item->data(kTypeXtypeRole).toString();
		const auto info = registry ? registry->typeInfo(xtype) : std::nullopt;
		const auto className = info ? info->className : QString();

		// Поиск по вхождению в xtype или полное имя класса.
		const bool match = query.isEmpty()
			|| xtype.contains(query, Qt::CaseInsensitive)
			|| className.contains(query, Qt::CaseInsensitive);
		item->setHidden(!match);

		if (!match) {
			continue;
		}
		if (!firstVisible) {
			firstVisible = item;
		}
		if (item == current) {
			currentVisible = true;
		}
	}

	if (currentVisible) {
		return;
	}

	typeList->setCurrentItem(firstVisible);
	if (!firstVisible) {
		updateDetails();
	}
}

void NewDocumentDialog::Private::selectType(QListWidgetItem* item) {
	Q_UNUSED(item);
	updateDetails();
}

void NewDocumentDialog::Private::updateDetails() {
	const auto type = currentType();
	const bool has = type.has_value();

	classNameValue->setText(has ? type->className : QStringLiteral("—"));
	xtypeValue->setText(has ? type->xtype : QStringLiteral("—"));
	singletoneValue->setText(has
		? (type->singletone ? NewDocumentDialog::tr("Да") : NewDocumentDialog::tr("Нет"))
		: QStringLiteral("—"));

	nameEdit->setEnabled(has && !type->singletone);
	nameRandomButton->setEnabled(has && !type->singletone);
	glyphEdit->setEnabled(has);
	titleEdit->setEnabled(has);

	if (!has) {
		nameEdit->clear();
		glyphEdit->clear();
		titleEdit->clear();
		nameHint->clear();
		fillDocumentVariables();
		updateAcceptState();
		return;
	}

	if (type->singletone) {
		nameEdit->setText(type->xtype);
		nameHint->setText(NewDocumentDialog::tr("Синглтон: имя всегда равно xtype"));
	}
	else {
		nameEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces));
		nameHint->setText(NewDocumentDialog::tr("Уникальное имя документа (id)"));
	}

	glyphEdit->setText(theme.glyphFor(type->className, type->xtype));
	titleEdit->setText(type->title.isEmpty() ? type->xtype : type->title);
	fillDocumentVariables();
	updateAcceptState();
}

void NewDocumentDialog::Private::updateAcceptState() {
	if (!buttons) {
		return;
	}

	auto* ok = buttons->button(QDialogButtonBox::Ok);
	if (!ok) {
		return;
	}

	const auto type = currentType();
	const bool hasName = type
		&& (type->singletone || !nameEdit->text().trimmed().isEmpty());
	ok->setEnabled(type.has_value() && hasName);
}

void NewDocumentDialog::Private::randomizeName() {
	const auto type = currentType();
	if (!type || type->singletone) {
		return;
	}

	nameEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces));
	nameEdit->setFocus(Qt::OtherFocusReason);
	nameEdit->selectAll();
}

void NewDocumentDialog::Private::acceptDialog(NewDocumentDialog* q) {
	const auto type = currentType();
	if (!type) {
		return;
	}

	if (!type->singletone) {
		const auto name = nameEdit->text().trimmed();
		if (name.isEmpty()) {
			return;
		}
		nameEdit->setText(name);
	}

	const auto title = titleEdit->text().trimmed();
	titleEdit->setText(title.isEmpty() ? type->xtype : title);
	q->accept();
}

QVariantMap NewDocumentDialog::Private::variables() const {
	return documentVariables;
}

std::optional<WidgetTypeHandler> NewDocumentDialog::Private::currentType() const {
	auto* item = typeList->currentItem();
	if (!item || !registry) {
		return std::nullopt;
	}

	return registry->typeInfo(item->data(kTypeXtypeRole).toString());
}

NewDocumentDialog::NewDocumentDialog(WidgetRegistry* registry,
	const std::shared_ptr<CommandContext>& context, QWidget* parent)
	: QDialog(parent)
	, d(std::make_unique<Private>()) {
	d->q = this;
	d->registry = registry;
	d->context = context;
	d->theme = loadTheme(context);
	d->setupUi(this);
	d->searchEdit->setFocus(Qt::OtherFocusReason);
}

NewDocumentDialog::~NewDocumentDialog() = default;

QString NewDocumentDialog::documentType() const {
	const auto type = d->currentType();
	return type ? type->xtype : QString();
}

QString NewDocumentDialog::documentName() const {
	const auto type = d->currentType();
	if (type && type->singletone) {
		return type->xtype;
	}

	return d->nameEdit->text().trimmed();
}

QString NewDocumentDialog::documentTitle() const {
	const auto title = d->titleEdit->text().trimmed();
	if (!title.isEmpty()) {
		return title;
	}

	const auto type = d->currentType();
	return type ? type->xtype : QString();
}

QString NewDocumentDialog::documentGlyph() const {
	return d->glyphEdit->text().trimmed();
}

bool NewDocumentDialog::isSingletone() const {
	const auto type = d->currentType();
	return type && type->singletone;
}

QVariantMap NewDocumentDialog::variables() const {
	return d->variables();
}
