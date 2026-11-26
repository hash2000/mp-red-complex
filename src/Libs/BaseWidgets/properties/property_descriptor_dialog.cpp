#include "Libs/BaseWidgets/properties/property_descriptor_dialog.h"
#include "Libs/BaseWidgets/properties/property_mime.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

namespace {

/// Добавляет mime-типы, которых ещё нет в списке (без учёта регистра).
void appendUnique(QStringList& target, const QStringList& values) {
	for (const auto& raw : values) {
		const auto mime = raw.trimmed();
		if (mime.isEmpty() || target.contains(mime, Qt::CaseInsensitive)) {
			continue;
		}

		target.append(mime);
	}
}

} // namespace

class PropertyDescriptorDialog::Private {
public:
	explicit Private(PropertyDescriptorDialog* parent)
		: q(parent) {
	}

	PropertyDescriptorDialog* q;
	QStringList currentMimeTypes;
	QStringList additionalMimeTypes;
	std::function<bool(const QString&)> nameValidator;

	QLineEdit* nameEdit = nullptr;
	QListWidget* mimeList = nullptr;
	QLineEdit* customEdit = nullptr;
	QToolButton* customAdd = nullptr;
	QPushButton* okButton = nullptr;

	void setupUi();
	void fillMimeList();
	void addCustomMime();
	void updateOk();
	QStringList suggestedMimeTypes() const;
	QStringList checkedMimeTypes() const;
};

void PropertyDescriptorDialog::Private::setupUi() {
	q->setObjectName(QStringLiteral("PropertyDescriptorDialog"));
	q->resize(420, 460);

	auto* layout = new QVBoxLayout(q);

	nameEdit = new QLineEdit(q);
	nameEdit->setObjectName(QStringLiteral("PropertyDescriptorName"));

	auto* form = new QFormLayout;
	form->addRow(PropertyDescriptorDialog::tr("Имя:"), nameEdit);
	layout->addLayout(form);

	auto* mimeGroup = new QGroupBox(PropertyDescriptorDialog::tr("Mime-типы"), q);
	auto* mimeLayout = new QVBoxLayout(mimeGroup);

	mimeList = new QListWidget(mimeGroup);
	mimeList->setObjectName(QStringLiteral("PropertyDescriptorMimeList"));

	auto* customRow = new QHBoxLayout;
	customEdit = new QLineEdit(mimeGroup);
	customEdit->setObjectName(QStringLiteral("PropertyDescriptorCustomMime"));
	customEdit->setPlaceholderText(PropertyDescriptorDialog::tr("Другой mime-тип"));
	customAdd = new QToolButton(mimeGroup);
	customAdd->setObjectName(QStringLiteral("PropertyDescriptorAddMime"));
	customAdd->setText(QStringLiteral("+"));
	customAdd->setToolTip(PropertyDescriptorDialog::tr("Добавить mime-тип"));
	customRow->addWidget(customEdit, 1);
	customRow->addWidget(customAdd);

	mimeLayout->addWidget(mimeList, 1);
	mimeLayout->addLayout(customRow);
	layout->addWidget(mimeGroup, 1);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q);
	layout->addWidget(buttons);
	QObject::connect(buttons, &QDialogButtonBox::accepted, q, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, q, &QDialog::reject);
	okButton = buttons->button(QDialogButtonBox::Ok);

	QObject::connect(customAdd, &QToolButton::clicked, q, [this] {
		addCustomMime();
	});
	QObject::connect(customEdit, &QLineEdit::returnPressed, q, [this] {
		addCustomMime();
	});
	QObject::connect(nameEdit, &QLineEdit::textChanged, q, [this](const QString&) {
		updateOk();
	});

	updateOk();
}

QStringList PropertyDescriptorDialog::Private::suggestedMimeTypes() const {
	QStringList ordered;
	appendUnique(ordered, currentMimeTypes);
	appendUnique(ordered, additionalMimeTypes);
	// Служебные пометки общие для свойств и переменных контекста: они
	// не задают редактор, но должны быть доступны в обоих диалогах.
	appendUnique(ordered, {
		QString::fromLatin1(PropertyMimeType::kShared),
		QString::fromLatin1(PropertyMimeType::kContextVariable)
	});
	appendUnique(ordered, PropertyMime::commonMimeTypes());
	return ordered;
}

void PropertyDescriptorDialog::Private::fillMimeList() {
	if (!mimeList) {
		return;
	}

	mimeList->clear();
	for (const auto& mimeType : suggestedMimeTypes()) {
		auto* item = new QListWidgetItem(mimeType, mimeList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

		bool checked = false;
		for (const auto& current : currentMimeTypes) {
			if (current.trimmed().compare(mimeType, Qt::CaseInsensitive) == 0) {
				checked = true;
				break;
			}
		}
		item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	}
}

void PropertyDescriptorDialog::Private::addCustomMime() {
	const auto value = customEdit->text().trimmed();
	if (value.isEmpty()) {
		return;
	}

	for (int row = 0; row < mimeList->count(); row++) {
		auto* item = mimeList->item(row);
		if (item->text().compare(value, Qt::CaseInsensitive) == 0) {
			item->setCheckState(Qt::Checked);
			mimeList->scrollToItem(item);
			customEdit->clear();
			return;
		}
	}

	auto* item = new QListWidgetItem(value, mimeList);
	item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
	item->setCheckState(Qt::Checked);
	mimeList->scrollToItem(item);
	customEdit->clear();
}

void PropertyDescriptorDialog::Private::updateOk() {
	if (!okButton || !nameEdit) {
		return;
	}

	const auto name = nameEdit->text().trimmed();
	const bool nameValid = !name.isEmpty() && (!nameValidator || nameValidator(name));
	okButton->setEnabled(nameValid);
}

QStringList PropertyDescriptorDialog::Private::checkedMimeTypes() const {
	QStringList result;
	if (!mimeList) {
		return result;
	}

	for (int row = 0; row < mimeList->count(); row++) {
		auto* item = mimeList->item(row);
		if (item->checkState() == Qt::Checked) {
			result.append(item->text().trimmed());
		}
	}

	return result;
}

PropertyDescriptorDialog::PropertyDescriptorDialog(QWidget* parent)
	: QDialog(parent)
	, d(std::make_unique<Private>(this)) {
	d->setupUi();
}

PropertyDescriptorDialog::~PropertyDescriptorDialog() = default;

void PropertyDescriptorDialog::setDialogTitle(const QString& title) {
	setWindowTitle(title);
}

void PropertyDescriptorDialog::setName(const QString& name) {
	d->nameEdit->setText(name);
}

QString PropertyDescriptorDialog::name() const {
	return d->nameEdit->text().trimmed();
}

void PropertyDescriptorDialog::setNameEditable(bool editable) {
	d->nameEdit->setEnabled(editable);
}

void PropertyDescriptorDialog::setMimeTypes(const QStringList& mimeTypes) {
	d->currentMimeTypes = mimeTypes;
	d->fillMimeList();
}

QStringList PropertyDescriptorDialog::mimeTypes() const {
	return d->checkedMimeTypes();
}

void PropertyDescriptorDialog::setAdditionalMimeTypes(const QStringList& mimeTypes) {
	d->additionalMimeTypes = mimeTypes;
	d->fillMimeList();
}

void PropertyDescriptorDialog::setNameValidator(std::function<bool(const QString&)> validator) {
	d->nameValidator = std::move(validator);
	d->updateOk();
}

std::optional<PropertyDescriptor> PropertyDescriptorDialog::request(QWidget* parent,
	const QString& title, const QString& name, const QStringList& mimeTypes, bool nameEditable,
	const QStringList& additionalMimeTypes, std::function<bool(const QString&)> nameValidator) {
	PropertyDescriptorDialog dialog(parent);
	dialog.setDialogTitle(title);
	dialog.setName(name);
	dialog.setNameEditable(nameEditable);
	dialog.setMimeTypes(mimeTypes);
	dialog.setAdditionalMimeTypes(additionalMimeTypes);
	dialog.setNameValidator(std::move(nameValidator));

	if (nameEditable) {
		dialog.d->nameEdit->setFocus(Qt::OtherFocusReason);
		dialog.d->nameEdit->selectAll();
	}
	else {
		dialog.d->mimeList->setFocus(Qt::OtherFocusReason);
	}

	if (dialog.exec() != QDialog::Accepted) {
		return std::nullopt;
	}

	PropertyDescriptor descriptor;
	descriptor.name = dialog.name();
	if (descriptor.name.isEmpty()) {
		return std::nullopt;
	}

	descriptor.mimeTypes = dialog.mimeTypes();
	return descriptor;
}
