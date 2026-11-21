#pragma once
#include "Libs/Engine/ui/doc_widget.h"

#include <QMetaObject>

/// Документ системных настроек.
///
/// Создаётся с ролью DocumentRule::SystemOptions: его контекст — `.options`
/// в режиме Mutable, поэтому изменения переменных документа попадают
/// в системные параметры глобального контекста. Обычные документы работают
/// в контексте `.documents` (только чтение) и менять настройки не могут.
class SystemOptionsDocument final : public DocumentWidget {
	Q_OBJECT
public:
	/// xtype документа в WidgetRegistry.
	static const QString& xtype();
	/// Полное имя класса документа в WidgetRegistry.
	static const QString& className();

	explicit SystemOptionsDocument(QWidget* parent = nullptr);

	QWidget* createPropertiesEditor(QWidget* parent) override;
	void refresh() override;

protected:
	void onContextAttached() override;
	void onContextVariableChanged(const QString& name, const QVariant& value) override;

private:
	/// Значения по умолчанию для ещё не заданных параметров.
	void ensureOptionVariables();
	/// Записывает значение параметра в контекст настроек.
	void applyOption(const QString& name, const QVariant& value);
	/// Возвращает все параметры к значениям по умолчанию.
	void resetOptions();

	/// true — значение пишется документом: редактор не перестраивается.
	bool _applying = false;
	/// Подписка на изменения глобального контекста (настройки меняются
	/// и снаружи, например командой из консоли).
	QMetaObject::Connection _settingsConnection;
};
