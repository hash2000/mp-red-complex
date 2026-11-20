#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QFileDialog>
#include <QStyle>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QColor>
#include <QColorDialog>
#include <QSpacerItem>

class PropertiesListWidget::Private {
public:
	Private(PropertiesListWidget* parent) : q(parent) {}
	PropertiesListWidget* q;

	void initUI();
	void rebuildUI(const QList<PropertyData>& properties);
	void clearUI();
	QWidget* createEditor(const PropertyData& prop);
	bool insertProperty(const PropertyData& prop, int row);

	QVBoxLayout* mainLayout = nullptr;
	QLabel* headerLabel = nullptr;
	QScrollArea* scrollArea = nullptr;
	QWidget* scrollContent = nullptr;
	QFormLayout* formLayout = nullptr;
	QSpacerItem* spacer = nullptr;
	QString basePath;
};

void PropertiesListWidget::Private::initUI() {
	mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	headerLabel = new QLabel("Properties", q);
	headerLabel->setObjectName(QStringLiteral("PropertiesHeaderLabel"));
	mainLayout->addWidget(headerLabel);

	scrollArea = new QScrollArea(q);
	scrollArea->setObjectName(QStringLiteral("PropertiesScroll"));
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);

	scrollContent = new QWidget();
	scrollContent->setObjectName(QStringLiteral("PropertiesContent"));
	formLayout = new QFormLayout(scrollContent);
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
	formLayout->setLabelAlignment(Qt::AlignLeft);
	formLayout->setFormAlignment(Qt::AlignLeft);
	formLayout->setVerticalSpacing(1);
	formLayout->setHorizontalSpacing(0);
	formLayout->setContentsMargins(0, 0, 0, 0);

	scrollArea->setWidget(scrollContent);
	mainLayout->addWidget(scrollArea);
}

void PropertiesListWidget::Private::clearUI() {
	if (!formLayout) {
		return;
	}

	// Удаляем все виджеты-редакторы
	for (int i = 0; i < formLayout->rowCount(); i++) {
		if (auto* item = formLayout->itemAt(i, QFormLayout::FieldRole)) {
			if (auto* w = item->widget()) {
				w->deleteLater();
			}
		}
		if (auto* item = formLayout->itemAt(i, QFormLayout::LabelRole)) {
			if (auto* l = item->widget()) {
				l->deleteLater();
			}
		}
	}
	// Сбрасываем layout
	delete formLayout;
	formLayout = nullptr;
	spacer = nullptr;

	// Создаём заново для чистого состояния
	formLayout = new QFormLayout(scrollContent);
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
	formLayout->setLabelAlignment(Qt::AlignLeft);
	formLayout->setFormAlignment(Qt::AlignLeft);
	formLayout->setVerticalSpacing(1);
	formLayout->setHorizontalSpacing(0);
	formLayout->setContentsMargins(0, 0, 0, 0);
}

void PropertiesListWidget::Private::rebuildUI(const QList<PropertyData>& properties) {
	clearUI();

	int row = 0;
	for (const auto& prop : properties) {
		if (insertProperty(prop, row)) {
			row++;
		}
	}

	// Распорка внизу
	spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
	formLayout->addItem(spacer);
}

bool PropertiesListWidget::Private::insertProperty(const PropertyData& prop, int row) {
	QWidget* editor = createEditor(prop);
	if (!editor) {
		return false;
	}

	auto* label = new QLabel(prop.label);
	label->setToolTip(prop.tooltip);
	formLayout->insertRow(row, label, editor);
	return true;
}


QWidget* PropertiesListWidget::Private::createEditor(const PropertyData& prop) {
	QWidget* editor = nullptr;

	switch (prop.type) {
		case PropertyType::Combo: {
			auto* combo = new QComboBox();
			combo->addItems(prop.comboOptions);
			int idx = prop.comboOptions.indexOf(prop.value.toString());
			if (idx >= 0) combo->setCurrentIndex(idx);

			combo->setObjectName(QStringLiteral("PropertyEditor"));
			combo->setProperty("readOnlyEditor", prop.readOnly);

			if (prop.readOnly) {
				combo->setEnabled(false);
			}
			else {
				QObject::connect(combo, &QComboBox::currentTextChanged, q,
					[this, id = prop.id](const QString& val) {
						emit q->propertyChanged(id, val);
					});
			}
			editor = combo;
			break;
		}

		case PropertyType::Text: {
			auto* lineEdit = new QLineEdit();
			lineEdit->setText(prop.value.toString());
			lineEdit->setReadOnly(prop.readOnly);
			lineEdit->setObjectName(QStringLiteral("PropertyEditor"));
			lineEdit->setProperty("readOnlyEditor", prop.readOnly);

			if (!prop.readOnly) {
				QObject::connect(lineEdit, &QLineEdit::textEdited, q,
					[this, id = prop.id](const QString& val) {
						emit q->propertyChanged(id, val);
					});
			}
			editor = lineEdit;
			break;
		}

		case PropertyType::Multiline: {
			auto* textEdit = new QTextEdit();
			textEdit->setPlainText(prop.value.toString());
			textEdit->setFixedHeight(60);
			textEdit->setReadOnly(prop.readOnly);
			textEdit->setTextInteractionFlags(prop.readOnly
				? Qt::TextSelectableByMouse
				: Qt::TextEditorInteraction);
			textEdit->setObjectName(QStringLiteral("PropertyEditor"));
			textEdit->setProperty("readOnlyEditor", prop.readOnly);

			if (!prop.readOnly) {
				QObject::connect(textEdit, &QTextEdit::textChanged, q,
					[this, id = prop.id, textEdit]() {
						emit q->propertyChanged(id, textEdit->toPlainText());
					});
			}
			editor = textEdit;
			break;
		}

		case PropertyType::PathFile:
		case PropertyType::PathDir: {
			auto* hLayout = new QHBoxLayout();
			hLayout->setContentsMargins(0, 0, 0, 0);
			hLayout->setSpacing(2);

			auto* pathEdit = new QLineEdit();
			pathEdit->setText(prop.value.toString());
			pathEdit->setReadOnly(true);
			pathEdit->setObjectName(QStringLiteral("PropertyEditor"));
			pathEdit->setProperty("readOnlyEditor", prop.readOnly);

			auto* btn = new QToolButton();
			btn->setObjectName(QStringLiteral("PropertyBrowseButton"));
			btn->setText("...");
			btn->setFixedWidth(24);
			btn->setToolTip(prop.type == PropertyType::PathFile ? "Выбрать файл" : "Выбрать папку");

			if (prop.readOnly) {
				btn->setEnabled(false);
			}

			else {
				QObject::connect(btn, &QToolButton::clicked, q,
					[this, pathEdit, prop]() {
						

						QString res;
						if (prop.type == PropertyType::PathFile) {
							res = QFileDialog::getOpenFileName(q, "Выберите файл",
								pathEdit->text(), prop.fileFilter);
						}
						else {
							res = QFileDialog::getExistingDirectory(q, "Выберите папку",
								pathEdit->text());
						}
						if (!res.isEmpty()) {
							pathEdit->setText(res);
							emit q->propertyChanged(prop.id, res);
						}
					});
			}

			hLayout->addWidget(pathEdit);
			hLayout->addWidget(btn);

			auto* container = new QWidget();
			container->setObjectName(QStringLiteral("PropertyContainer"));
			container->setLayout(hLayout);
			editor = container;
			break;
		}

		case PropertyType::Boolean: {
			auto* chk = new QCheckBox();
			chk->setChecked(prop.value.toBool());
			chk->setEnabled(!prop.readOnly);

			auto* wrapper = new QWidget();
			wrapper->setObjectName(QStringLiteral("PropertyContainer"));
			auto* wLayout = new QHBoxLayout(wrapper);
			wLayout->addWidget(chk, 0, Qt::AlignCenter);
			wLayout->setContentsMargins(0, 0, 0, 0);
			wrapper->setLayout(wLayout);

			if (!prop.readOnly) {
				QObject::connect(chk, &QCheckBox::toggled, q,
					[this, id = prop.id](bool val) {
						emit q->propertyChanged(id, val);
					});
			}
			editor = wrapper;
			break;
		}

		case PropertyType::Number: {
			bool isDouble = (prop.step != static_cast<int>(prop.step)) ||
				(prop.minVal != static_cast<int>(prop.minVal));

			if (isDouble) {
				auto* spin = new QDoubleSpinBox();
				spin->setValue(prop.value.toDouble());
				spin->setRange(prop.minVal, prop.maxVal);
				spin->setSingleStep(prop.step);
				spin->setReadOnly(prop.readOnly); // setReadOnly() у QAbstractSpinBox есть
				spin->setObjectName(QStringLiteral("PropertyEditor"));
				spin->setProperty("readOnlyEditor", prop.readOnly);

				if (!prop.readOnly) {
					QObject::connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), q,
						[this, id = prop.id](double val) {
							emit q->propertyChanged(id, val);
						});
				}
				editor = spin;
			}
			else {
				auto* spin = new QSpinBox();
				spin->setValue(prop.value.toInt());
				spin->setRange(static_cast<int>(prop.minVal), static_cast<int>(prop.maxVal));
				spin->setSingleStep(static_cast<int>(prop.step));
				spin->setReadOnly(prop.readOnly);
				spin->setObjectName(QStringLiteral("PropertyEditor"));
				spin->setProperty("readOnlyEditor", prop.readOnly);

				if (!prop.readOnly) {
					QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), q,
						[this, id = prop.id](int val) {
							emit q->propertyChanged(id, val);
						});
				}
				editor = spin;
			}
			break;
		}
		case PropertyType::Button: {
			auto* button = new QToolButton();
			button->setToolTip(prop.tooltip);
			button->setText("...");
			button->setToolButtonStyle(Qt::ToolButtonTextOnly);
			button->setEnabled(!prop.readOnly);
			button->setObjectName(QStringLiteral("PropertyEditor"));
			button->setProperty("readOnlyEditor", prop.readOnly);

			if (!prop.readOnly) {
				QObject::connect(button, &QToolButton::clicked, q,
					[this, id = prop.id]() {
						emit q->propertyButtonClick(id);
					});
			}

			editor = button;
			break;
		}
		case PropertyType::Color: {
			auto* colorBtn = new QPushButton();
			QColor initialColor = prop.value.value<QColor>();
			if (!initialColor.isValid()) {
				initialColor = Qt::black;
			}

			colorBtn->setObjectName(QStringLiteral("PropertyColorButton"));
			colorBtn->setText(initialColor.name());
			colorBtn->setFixedHeight(26);
			colorBtn->setCursor(Qt::PointingHandCursor);

			// Рамка и отступы кнопки заданы темой; код выставляет только сам цвет,
			// потому что это значение редактируемого свойства.
			auto updateColorStyle = [colorBtn](const QColor& c) {
				bool isLight = c.lightness() > 128;
				QString textColor = isLight ? "#000000" : "#FFFFFF";
				QString style = QString(
					"background-color: %1; color: %2;"
				).arg(c.name()).arg(textColor);
				colorBtn->setStyleSheet(style);
			};
			updateColorStyle(initialColor);
			colorBtn->setProperty("currentColor", initialColor);

			if (!prop.readOnly) {
				QObject::connect(colorBtn, &QPushButton::clicked, q,
					[this, colorBtn, prop, updateColorStyle]() {
					QColor current = colorBtn->property("currentColor").value<QColor>();
					QColorDialog dlg(current, q);
					dlg.setOption(QColorDialog::ShowAlphaChannel, true); // Опционально: поддержка прозрачности
					if (dlg.exec() == QDialog::Accepted) {
						QColor newColor = dlg.selectedColor();
						colorBtn->setProperty("currentColor", newColor);
						colorBtn->setText(newColor.name());
						updateColorStyle(newColor);

						emit q->propertyChanged(prop.id, newColor);
					}
				});
			}
			else {
				colorBtn->setEnabled(false);
			}

			editor = colorBtn;
			break;
		}
	}

	if (editor) {
		editor->setProperty("propertyId", prop.id);
	}

	return editor;
}

PropertiesListWidget::PropertiesListWidget(QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->initUI();
}

PropertiesListWidget::~PropertiesListWidget() = default;

void PropertiesListWidget::setProperties(const QList<PropertyData>& properties) {
	d->rebuildUI(properties);
}

void PropertiesListWidget::addProperty(const PropertyData& property) {
	// Новое свойство вставляется перед распоркой, чтобы она осталась внизу.
	int row = d->formLayout->rowCount();
	if (d->spacer) {
		row = qMax(0, row - 1);
	}

	d->insertProperty(property, row);
}

bool PropertiesListWidget::removeProperty(const QString& propertyId) {
	if (!d->formLayout || propertyId.isEmpty()) {
		return false;
	}

	for (int row = 0; row < d->formLayout->rowCount(); row++) {
		auto* item = d->formLayout->itemAt(row, QFormLayout::FieldRole);
		auto* editor = item ? item->widget() : nullptr;
		if (!editor || editor->property("propertyId").toString() != propertyId) {
			continue;
		}

		// removeRow удаляет и подпись, и редактор свойства.
		d->formLayout->removeRow(row);
		return true;
	}

	return false;
}

void PropertiesListWidget::setHeaderTitle(const QString& title) {
	if (d->headerLabel) {
		d->headerLabel->setText(title);
	}
}

void PropertiesListWidget::clearProperties() {
	d->clearUI();
}

QStringList PropertiesListWidget::propertyIds() const {
	QStringList result;
	if (!d->formLayout) {
		return result;
	}

	for (int row = 0; row < d->formLayout->rowCount(); row++) {
		auto* item = d->formLayout->itemAt(row, QFormLayout::FieldRole);
		auto* editor = item ? item->widget() : nullptr;
		if (!editor) {
			continue;
		}

		const auto id = editor->property("propertyId").toString();
		if (!id.isEmpty()) {
			result.append(id);
		}
	}

	return result;
}

int PropertiesListWidget::propertyCount() const {
	return propertyIds().size();
}

void PropertiesListWidget::setBasePath(const QString& path) {
	d->basePath = path;
}
