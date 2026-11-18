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
#include <QSplitter>
#include <QTableWidget>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>

#include <optional>

namespace {
/// Роль элемента списка: xtype зарегистрированного типа.
constexpr int kTypeXtypeRole = Qt::UserRole + 1;
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

	QTableWidget* variablesTable = nullptr;
	QDialogButtonBox* buttons = nullptr;

	void setupUi(NewDocumentDialog* q);
	void fillTypes();
	void fillVariables();
	void applyFilter(const QString& text);
	void updateDetails();
	void updateAcceptState();
	void randomizeName();
	void selectType(QListWidgetItem* item);
	void acceptDialog(NewDocumentDialog* q);
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

	// Переменные текущего контекста, в котором создаётся документ.
	const auto namespaceName = context && !context->isGlobalContext()
		? context->nameSpace() : QStringLiteral(".");
	auto* variablesGroup = new QGroupBox(
		NewDocumentDialog::tr("Переменные контекста %1").arg(namespaceName), right);
	auto* variablesLayout = new QVBoxLayout(variablesGroup);
	variablesLayout->setContentsMargins(6, 6, 6, 6);

	variablesTable = new QTableWidget(0, 3, variablesGroup);
	variablesTable->setObjectName(QStringLiteral("NewDocumentDialogVariables"));
	variablesTable->setHorizontalHeaderLabels({ NewDocumentDialog::tr("Имя"),
		NewDocumentDialog::tr("Значение"), NewDocumentDialog::tr("Тип") });
	variablesTable->verticalHeader()->setVisible(false);
	variablesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	variablesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	variablesTable->setAlternatingRowColors(true);
	variablesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	variablesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	variablesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	variablesLayout->addWidget(variablesTable);

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
	variablesTable->setRowCount(0);
	if (!context) {
		return;
	}

	const auto variables = context->variables();
	variablesTable->setRowCount(static_cast<int>(variables.size()));

	int row = 0;
	for (auto it = variables.cbegin(); it != variables.cend(); it++, row++) {
		auto* nameItem = new QTableWidgetItem(it.key());
		nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

		auto* valueItem = new QTableWidgetItem(it.value().toString());
		valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
		valueItem->setToolTip(it.value().toString());

		auto* typeItem = new QTableWidgetItem(QString::fromLatin1(it.value().typeName()));
		typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);

		variablesTable->setItem(row, 0, nameItem);
		variablesTable->setItem(row, 1, valueItem);
		variablesTable->setItem(row, 2, typeItem);
	}
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
