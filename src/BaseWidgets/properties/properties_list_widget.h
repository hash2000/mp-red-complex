#pragma once
#include <QWidget>
#include <QVariant>
#include <QString>
#include <QList>
#include <memory>

enum class PropertyType {
	Combo,      // Выпадающий список
	Text,       // Однострочный текст
	Multiline,  // Многострочный текст
	PathFile,   // Выбор файла
	PathDir,    // Выбор директории
	Boolean,    // Чекбокс true/false
	Number,     // Целое или вещественное число
	Button,     // Кнопка
	Color,      // Выбор цвета
};

struct PropertyData {
	QString id;                    // Уникальный идентификатор свойства
	QString label;                 // Отображаемое имя
	PropertyType type;             // Тип редактора
	QVariant value;                // Текущее значение
	QVariant defaultValue;         // Значение по умолчанию
	bool readOnly = false;         // Только чтение (текст выделяется, редактирование запрещено)
	QStringList comboOptions;      // Варианты для Combo
	QString fileFilter;            // Фильтр диалога файлов (напр. "*.png *.jpg")
	QString tooltip;               // Подсказка
	double minVal = 0;             // Минимум для Number
	double maxVal = 100;           // Максимум для Number
	double step = 1;               // Шаг для Number
};

class PropertiesListWidget : public QWidget {
	Q_OBJECT
public:
	explicit PropertiesListWidget(QWidget* parent = nullptr);
	~PropertiesListWidget() override;

	/// Заменяет текущий набор свойств на новый (перестраивает UI)
	void setProperties(const QList<PropertyData>& properties);

	/// Изменяет заголовок панели
	void setHeaderTitle(const QString& title);

	/// Удалить все свойства
	void clearProperties();

signals:
	/// Испускается при изменении любого свойства
	/// @param propertyId Идентификатор свойства
	/// @param newValue   Новое значение (QString, bool, int, double)
	void propertyChanged(const QString& propertyId, const QVariant& newValue);

	/// Нажатие на кнопку
	void propertyButtonClick(const QString& propertyId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
