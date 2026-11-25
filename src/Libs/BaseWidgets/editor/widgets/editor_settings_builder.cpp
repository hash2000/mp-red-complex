#include "Libs/BaseWidgets/editor/widgets/editor_settings_builder.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QSignalBlocker>
#include <QWidget>

namespace {
const QList<int> kFontSizes = {
	6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36, 48, 72
};
} // namespace

EditorSettingsBuilder::EditorSettingsBuilder(QWidget* parent)
	: parent(parent) {
}

QComboBox* EditorSettingsBuilder::addFontSizeRow(QFormLayout* form, int currentSize,
	const std::function<void(int)>& onChange) {
	auto* combo = new QComboBox(parent);
	combo->setObjectName(QStringLiteral("EditorFontSizeCombo"));
	combo->setToolTip(QCoreApplication::translate("EditorSettingsBuilder", "Размер шрифта редактора"));
	combo->setFixedWidth(76);
	for (const int size : kFontSizes) {
		combo->addItem(QString::number(size), size);
	}

	int index = combo->findData(currentSize);
	if (index < 0) {
		combo->addItem(QString::number(currentSize), currentSize);
		index = combo->count() - 1;
	}
	combo->setCurrentIndex(index);

	QObject::connect(combo, &QComboBox::currentIndexChanged, combo,
		[combo, onChange](int row) {
			if (row >= 0 && onChange) {
				onChange(combo->itemData(row).toInt());
			}
		});

	form->addRow(QCoreApplication::translate("EditorSettingsBuilder", "Aa  Размер шрифта:"), combo);
	return combo;
}

QComboBox* EditorSettingsBuilder::addLineWrapRow(QFormLayout* form, int currentValue,
	bool allowAuto, const std::function<void(int)>& onChange) {
	auto* combo = new QComboBox(parent);
	combo->setObjectName(QStringLiteral("EditorLineWrapCombo"));
	combo->setToolTip(QCoreApplication::translate("EditorSettingsBuilder", "Перенос строк"));

	if (allowAuto) {
		combo->addItem(QCoreApplication::translate("EditorSettingsBuilder", "Авто (длинные строки)"), 0);
		combo->addItem(QCoreApplication::translate("EditorSettingsBuilder", "Включён"), 1);
		combo->addItem(QCoreApplication::translate("EditorSettingsBuilder", "Выключен"), 2);
	}
	else {
		combo->addItem(QCoreApplication::translate("EditorSettingsBuilder", "Включён"), 1);
		combo->addItem(QCoreApplication::translate("EditorSettingsBuilder", "Выключен"), 0);
	}

	const int index = combo->findData(currentValue);
	combo->setCurrentIndex(index >= 0 ? index : 0);

	QObject::connect(combo, &QComboBox::currentIndexChanged, combo,
		[combo, onChange](int row) {
			if (row >= 0 && onChange) {
				onChange(combo->itemData(row).toInt());
			}
		});

	form->addRow(QCoreApplication::translate("EditorSettingsBuilder", "¶  Перенос строк:"), combo);
	return combo;
}

QCheckBox* EditorSettingsBuilder::addLineNumbersRow(QFormLayout* form,
	const QString& objectName, bool checked, const std::function<void(bool)>& onChange) {
	auto* check = new QCheckBox(parent);
	check->setObjectName(objectName);
	check->setToolTip(QCoreApplication::translate("EditorSettingsBuilder", "Показывать номера строк"));
	check->setChecked(checked);
	QObject::connect(check, &QCheckBox::toggled, check, [onChange](bool value) {
		if (onChange) {
			onChange(value);
		}
	});

	form->addRow(QCoreApplication::translate("EditorSettingsBuilder", "#  Номера строк:"), check);
	return check;
}
