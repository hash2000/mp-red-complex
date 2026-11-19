#include "Libs/Engine/ui/new_document_dialog.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>

#include <algorithm>
#include <optional>

namespace {
/// Роль элемента списка: xtype зарегистрированного типа.
constexpr int kTypeXtypeRole = Qt::UserRole + 1;
/// Роль ячейки значения: исходный QVariant переменной.
constexpr int kValueRole = Qt::UserRole + 1;

/// Значение из текстовой ячейки: bool, целое, дробное или строка.
QVariant parseVariableValue(const QString& text) {
	if (text.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0) {
		return true;
	}
	if (text.compare(QLatin1String("false"), Qt::CaseInsensitive) == 0) {
		return false;
	}

	bool ok = false;
	const auto integer = text.toLongLong(&ok);
	if (ok) {
		return integer;
	}

	const auto real = text.toDouble(&ok);
	if (ok) {
		return real;
	}

	return text;
}
} // namespace

class NewDocumentDialog::Private {
public:
	WidgetRegistry* registry = nullptr;
	std::shared_ptr<CommandContext> context;

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
	QTableWidget* variablesTable = nullptr;
	QDialogButtonBox* buttons = nullptr;

	void setupUi(NewDocumentDialog* q);
	void fillTypes();
	void fillVariables();
	void addVariableRow();
	void removeSelectedVariables();
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

	// Переменные, которые попадут в контекст создаваемого документа.
	// Начальные строки берутся из текущего контекста и редактируются.
	const auto namespaceName = context && !context->isGlobalContext()
		? context->nameSpace() : QStringLiteral(".");
	auto* variablesGroup = new QGroupBox(
		NewDocumentDialog::tr("Переменные контекста %1").arg(namespaceName), right);
	auto* variablesLayout = new QVBoxLayout(variablesGroup);
	variablesLayout->setContentsMargins(6, 6, 6, 6);

	auto* variablesTools = new QHBoxLayout;
	variablesTools->setContentsMargins(0, 0, 0, 0);
	variablesTools->setSpacing(4);

	addVariableButton = new QToolButton(variablesGroup);
	addVariableButton->setObjectName(QStringLiteral("NewDocumentDialogAddVariable"));
	addVariableButton->setText(QStringLiteral("+"));
	addVariableButton->setToolTip(NewDocumentDialog::tr("Добавить переменную"));
	removeVariableButton = new QToolButton(variablesGroup);
	removeVariableButton->setObjectName(QStringLiteral("NewDocumentDialogRemoveVariable"));
	removeVariableButton->setText(QStringLiteral("−"));
	removeVariableButton->setToolTip(NewDocumentDialog::tr("Удалить выбранные переменные"));

	variablesTools->addWidget(addVariableButton);
	variablesTools->addWidget(removeVariableButton);
	variablesTools->addStretch(1);

	auto* variablesHint = new QLabel(
		NewDocumentDialog::tr("Начальные переменные контекста документа"), variablesGroup);
	variablesHint->setWordWrap(true);

	variablesTable = new QTableWidget(0, 3, variablesGroup);
	variablesTable->setObjectName(QStringLiteral("NewDocumentDialogVariables"));
	variablesTable->setHorizontalHeaderLabels({ NewDocumentDialog::tr("Имя"),
		NewDocumentDialog::tr("Значение"), NewDocumentDialog::tr("Тип") });
	variablesTable->verticalHeader()->setVisible(false);
	variablesTable->setEditTriggers(QAbstractItemView::DoubleClicked
		| QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
	variablesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	variablesTable->setAlternatingRowColors(true);
	variablesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	variablesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	variablesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	variablesLayout->addWidget(variablesHint);
	variablesLayout->addLayout(variablesTools);
	variablesLayout->addWidget(variablesTable, 1);

	rightLayout->addWidget(infoGroup);
	rightLayout->addWidget(paramsGroup);
	rightLayout->addWidget(variablesGroup, 1);

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
		addVariableRow();
	});
	QObject::connect(removeVariableButton, &QToolButton::clicked, q, [this] {
		removeSelectedVariables();
	});
	QObject::connect(variablesTable, &QTableWidget::itemChanged, q,
		[this](QTableWidgetItem* item) {
			if (!item || item->column() != 1) {
				return;
			}

			// Отображаемый тип и записываемое значение следуют за текстом.
			const QSignalBlocker blocker(variablesTable);
			const auto value = parseVariableValue(item->text());
			item->setData(kValueRole, value);
			item->setToolTip(item->text());
			if (auto* typeItem = variablesTable->item(item->row(), 2)) {
				typeItem->setText(QString::fromLatin1(value.typeName()));
			}
		});
	QObject::connect(buttons, &QDialogButtonBox::accepted, q, [this, q] {
		acceptDialog(q);
	});
	QObject::connect(buttons, &QDialogButtonBox::rejected, q, &QDialog::reject);

	fillTypes();
	fillVariables();
	if (typeList->count() == 0) {
		updateDetails();
	}
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

void NewDocumentDialog::Private::fillVariables() {
	const QSignalBlocker blocker(variablesTable);
	variablesTable->setRowCount(0);
	if (!context) {
		return;
	}

	const auto variables = context->variables();
	variablesTable->setRowCount(static_cast<int>(variables.size()));

	int row = 0;
	for (auto it = variables.cbegin(); it != variables.cend(); it++, row++) {
		auto* nameItem = new QTableWidgetItem(it.key());

		auto* valueItem = new QTableWidgetItem(it.value().toString());
		valueItem->setData(kValueRole, it.value());
		valueItem->setToolTip(it.value().toString());

		auto* typeItem = new QTableWidgetItem(QString::fromLatin1(it.value().typeName()));
		typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);

		variablesTable->setItem(row, 0, nameItem);
		variablesTable->setItem(row, 1, valueItem);
		variablesTable->setItem(row, 2, typeItem);
	}
}

void NewDocumentDialog::Private::addVariableRow() {
	const QSignalBlocker blocker(variablesTable);
	const int row = variablesTable->rowCount();
	variablesTable->insertRow(row);

	auto* nameItem = new QTableWidgetItem(NewDocumentDialog::tr("variable"));
	auto* valueItem = new QTableWidgetItem(QString());
	valueItem->setData(kValueRole, QString());

	auto* typeItem = new QTableWidgetItem(
		QString::fromLatin1(QVariant(QString()).typeName()));
	typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);

	variablesTable->setItem(row, 0, nameItem);
	variablesTable->setItem(row, 1, valueItem);
	variablesTable->setItem(row, 2, typeItem);
	variablesTable->setCurrentCell(row, 1);
}

void NewDocumentDialog::Private::removeSelectedVariables() {
	QSet<int> selectedRows;
	for (auto* item : variablesTable->selectedItems()) {
		selectedRows.insert(item->row());
	}
	if (selectedRows.isEmpty() && variablesTable->currentRow() >= 0) {
		selectedRows.insert(variablesTable->currentRow());
	}

	QList<int> rows = selectedRows.values();
	std::sort(rows.begin(), rows.end());
	for (auto it = rows.crbegin(); it != rows.crend(); it++) {
		variablesTable->removeRow(*it);
	}
}

QVariantMap NewDocumentDialog::Private::variables() const {
	QVariantMap result;
	for (int row = 0; row < variablesTable->rowCount(); row++) {
		auto* nameItem = variablesTable->item(row, 0);
		if (!nameItem) {
			continue;
		}

		const auto name = nameItem->text().trimmed();
		if (name.isEmpty()) {
			continue;
		}

		QVariant value;
		if (auto* valueItem = variablesTable->item(row, 1)) {
			value = valueItem->data(kValueRole);
			if (!value.isValid()) {
				value = parseVariableValue(valueItem->text());
			}
		}

		result.insert(name, value);
	}

	return result;
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

	glyphEdit->setText(type->glyph);
	titleEdit->setText(type->title.isEmpty() ? type->xtype : type->title);
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
	d->registry = registry;
	d->context = context;
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
