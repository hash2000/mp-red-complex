#pragma once
#include <QString>
#include <functional>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QWidget;

/// Общие параметры редактора для панели «Свойства»: размер шрифта,
/// перенос строк и номера строк. Одинаково используется редактором кода
/// и просмотрщиком больших файлов.
class EditorSettingsBuilder {
public:
	explicit EditorSettingsBuilder(QWidget* parent);

	QComboBox* addFontSizeRow(QFormLayout* form, int currentSize,
		const std::function<void(int)>& onChange);
	/// allowAuto — вариант «Авто» (для CodeEditorWidget); иначе только
	/// «Включён»/«Выключен».
	QComboBox* addLineWrapRow(QFormLayout* form, int currentValue, bool allowAuto,
		const std::function<void(int)>& onChange);
	QCheckBox* addLineNumbersRow(QFormLayout* form, const QString& objectName,
		bool checked, const std::function<void(bool)>& onChange);

private:
	QWidget* parent = nullptr;
};
