#pragma once
#include <QColor>
#include <QDialog>
#include <QVariant>

#include <memory>

class WidgetRegistry;
class CommandContext;

/// Диалог создания документа. Слева — поиск по xtype/имени класса и список
/// зарегистрированных типов, справа — сведения о типе, параметры создания
/// (name, glyph, title) и обязательные переменные выбранного документа.
///
/// Переменные берутся из IDocumentMetadata, зарегистрированных в WidgetRegistry
/// для класса документа. По mime-типу переменной выбирается редактор значения
/// в PropertiesListWidget.
///
/// Документ создаёт вызывающая сторона (MainWindow::Private::newDocument):
/// диалог лишь возвращает выбранные значения. Для типа-синглтона имя
/// всегда равно xtype и не редактируется.
class NewDocumentDialog : public QDialog {
	Q_OBJECT
public:
	/// @param registry реестр типов виджетов: список типов, значения по умолчанию
	///        и метаданные документов
	/// @param context контекст, в котором создаётся документ; из него читаются
	///        начальные значения переменных (может быть nullptr)
	NewDocumentDialog(WidgetRegistry* registry, const std::shared_ptr<CommandContext>& context,
		QWidget* parent = nullptr);
	~NewDocumentDialog() override;

	QString documentType() const;    ///< xtype выбранного типа
	QString documentName() const;    ///< имя (id) документа
	QString documentTitle() const;   ///< подпись документа
	QString documentGlyph() const;   ///< иконка UTF-8
	/// Цвет вкладки из параметров диалога; невалидный — цвет из темы.
	QColor documentColor() const;
	bool isSingletone() const;       ///< выбран тип-синглтон

	/// Обязательные переменные выбранного документа: при создании они
	/// записываются в его контекст до инициализации. Выбранный цвет вкладки
	/// передаётся переменной `color`.
	QVariantMap variables() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
