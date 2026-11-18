#pragma once
#include <QDialog>

#include <memory>

class WidgetRegistry;
class CommandContext;

/// Диалог создания документа. Слева — поиск по xtype/имени класса и список
/// зарегистрированных типов, справа — сведения о типе, параметры создания
/// (name, glyph, title) и переменные текущего контекста.
///
/// Документ создаёт вызывающая сторона (MainWindow::Private::newDocument):
/// диалог лишь возвращает выбранные значения. Для типа-синглтона имя
/// всегда равно xtype и не редактируется.
class NewDocumentDialog : public QDialog {
	Q_OBJECT
public:
	/// @param registry реестр типов виджетов: список типов и значения по умолчанию
	/// @param context контекст, в котором создаётся документ; его переменные
	///        показываются в таблице (может быть nullptr)
	NewDocumentDialog(WidgetRegistry* registry, const std::shared_ptr<CommandContext>& context,
		QWidget* parent = nullptr);
	~NewDocumentDialog() override;

	QString documentType() const;    ///< xtype выбранного типа
	QString documentName() const;    ///< имя (id) документа
	QString documentTitle() const;   ///< подпись документа
	QString documentGlyph() const;   ///< иконка UTF-8
	bool isSingletone() const;       ///< выбран тип-синглтон

private:
	class Private;
	std::unique_ptr<Private> d;
};
