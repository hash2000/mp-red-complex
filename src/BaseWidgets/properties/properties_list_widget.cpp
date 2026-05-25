#include "BaseWidgets/properties/properties_list_widget.h"
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

class PropertiesListWidget::Private {
public:
	Private(PropertiesListWidget* parent) : q(parent) {}
	PropertiesListWidget* q;

	void initUI();
	void rebuildUI(const QList<PropertyData>& properties);
	void clearUI();
	QWidget* createEditor(const PropertyData& prop);
	QString getEditorStyle(bool readOnly) const;

	QVBoxLayout* mainLayout = nullptr;
	QLabel* headerLabel = nullptr;
	QScrollArea* scrollArea = nullptr;
	QWidget* scrollContent = nullptr;
	QFormLayout* formLayout = nullptr;
};

void PropertiesListWidget::Private::initUI() {
	mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	headerLabel = new QLabel("Properties", q);
	headerLabel->setStyleSheet(
		"background: #2D2D30; color: #CCCCCC; padding: 6px 8px; "
		"font-weight: bold; border-bottom: 1px solid #3E3E42;"
	);
	mainLayout->addWidget(headerLabel);

	scrollArea = new QScrollArea(q);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setStyleSheet("background: transparent; border: none;");

	scrollContent = new QWidget();
	formLayout = new QFormLayout(scrollContent);
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	formLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
	formLayout->setLabelAlignment(Qt::AlignLeft);
	formLayout->setFormAlignment(Qt::AlignLeft);
	formLayout->setVerticalSpacing(1);
	formLayout->setHorizontalSpacing(0);
	formLayout->setContentsMargins(0, 0, 0, 0);

	scrollContent->setStyleSheet(
		"QLabel { padding: 4px 6px; background: #252526; color: #D4D4D4; "
		"border-bottom: 1px solid #3E3E42; }"
	);

	scrollArea->setWidget(scrollContent);
	mainLayout->addWidget(scrollArea);
}

void PropertiesListWidget::Private::clearUI() {
	if (!formLayout) return;

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

	for (const auto& prop : properties) {
		QWidget* editor = createEditor(prop);
		if (!editor) {
			continue;
		}

		auto* label = new QLabel(prop.label);
		label->setToolTip(prop.tooltip);
		formLayout->addRow(label, editor);
	}

	// Распорка внизу
	auto* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
	formLayout->addItem(spacer);
}


QWidget* PropertiesListWidget::Private::createEditor(const PropertyData& prop) {
	QWidget* editor = nullptr;
	const QString style = getEditorStyle(prop.readOnly);

	switch (prop.type) {
		case PropertyType::Combo: {
			auto* combo = new QComboBox();
			combo->addItems(prop.comboOptions);
			int idx = prop.comboOptions.indexOf(prop.value.toString());
			if (idx >= 0) combo->setCurrentIndex(idx);

			if (prop.readOnly) {
				combo->setEnabled(false);
				combo->setStyleSheet(style);
			}
			else {
				combo->setStyleSheet(style);
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
			lineEdit->setStyleSheet(style);

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
			textEdit->setStyleSheet(style);

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
			pathEdit->setStyleSheet(style);

			auto* btn = new QToolButton();
			btn->setText("...");
			btn->setFixedWidth(24);
			btn->setToolTip(prop.type == PropertyType::PathFile ? "Выбрать файл" : "Выбрать папку");
			btn->setStyleSheet("QToolButton { background: #3E3E42; color: white; border: none; } "
				"QToolButton:pressed { background: #007ACC; } "
				"QToolButton:disabled { background: #333333; color: #666666; }");

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
			container->setLayout(hLayout);
			container->setStyleSheet("background: transparent;");
			editor = container;
			break;
		}

		case PropertyType::Boolean: {
			auto* chk = new QCheckBox();
			chk->setChecked(prop.value.toBool());
			chk->setEnabled(!prop.readOnly);
			if (prop.readOnly) {
				chk->setStyleSheet(style);
			}

			auto* wrapper = new QWidget();
			auto* wLayout = new QHBoxLayout(wrapper);
			wLayout->addWidget(chk, 0, Qt::AlignCenter);
			wLayout->setContentsMargins(0, 0, 0, 0);
			wrapper->setLayout(wLayout);
			wrapper->setStyleSheet("background: transparent;");

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
				spin->setStyleSheet(style);

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
				spin->setStyleSheet(style);

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
			button->setStyleSheet(style);

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

			colorBtn->setText(initialColor.name());
			colorBtn->setFixedHeight(26);
			colorBtn->setCursor(Qt::PointingHandCursor);

			// Лямбда для динамического обновления стиля под текущий цвет
			auto updateColorStyle = [colorBtn](const QColor& c) {
				bool isLight = c.lightness() > 128;
				QString textColor = isLight ? "#000000" : "#FFFFFF";
				QString style = QString(
					"QPushButton { background-color: %1; color: %2; border: 1px solid #3E3E42; padding: 2px 6px; text-align: left; }"
					"QPushButton:hover { border-color: #007ACC; }"
					"QPushButton:disabled { border-color: transparent; opacity: 0.75; }"
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

QString PropertiesListWidget::Private::getEditorStyle(bool readOnly) const {
	if (readOnly) {
		return R"(
            background: #252526; color: #808080; border: 1px solid transparent; 
            padding: 2px 4px; selection-background-color: #555555;
        )";
	}
	return R"(
        background: #1E1E1E; color: #D4D4D4; border: 1px solid #3E3E42; 
        padding: 2px 4px; selection-background-color: #094771;
    )";
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

void PropertiesListWidget::setHeaderTitle(const QString& title) {
	if (d->headerLabel) {
		d->headerLabel->setText(title);
	}
}

void PropertiesListWidget::clearProperties() {
	d->clearUI();
}
