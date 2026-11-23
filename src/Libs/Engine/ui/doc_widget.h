#pragma once
#include <QColor>
#include <QList>
#include <QString>
#include <QWidget>
#include <QVariant>
#include <memory>

class CommandContext;

/// Группа свойств документа для панели «Свойства».
///
/// Документ отдаёт такие группы из `createPropertyGroups()`; панель показывает
/// их вложенными в раздел «Свойства документа». Например, у редактора кода это
/// «Параметры поиска», которые раскрывает значок свойств у кнопки ленты.
struct DocumentPropertyGroup {
	QString id;                 ///< стабильный идентификатор ("search")
	QString title;              ///< подпись группы
	QWidget* editor = nullptr;  ///< содержимое; nullptr — пустая группа
	bool expanded = false;      ///< раскрыта ли группа по умолчанию
};

/// Базовый класс для всех виджетов-документов.
/// Каждому DocumentWidget автоматически соответствует вкладка слева.
/// Документ связан с остальными документами через свой CommandContext
/// (`.documents.{id}`), поэтому изменение свойств и вызовы видны соседям.
///
/// Состояние документа хранится в переменных его контекста: `title`, `glyph`,
/// `modified`, `color`, `xtype`, `className` и переменные конкретного документа
/// (например, `path`).
class DocumentWidget : public QWidget {
	Q_OBJECT
public:
	explicit DocumentWidget(QWidget* parent = nullptr);
	~DocumentWidget() override;

	QString title() const;
	void setTitle(const QString& title);

	QString glyph() const;               // UTF-8 значок вкладки (действующий)
	/// Явный глиф документа; пустая строка — глиф не задан, берётся из темы
	/// или глиф по умолчанию.
	void setGlyph(const QString& glyph);

	/// Стандартный глиф из темы (привязка к классу документа или xtype).
	/// Не хранится в контексте: явный глиф документа важнее.
	QString themeGlyph() const;
	void setThemeGlyph(const QString& glyph);

	/// Глиф по умолчанию, если ни документ, ни тема его не задали.
	static const QString& defaultGlyph();

	bool isModified() const;
	void setModified(bool modified);

	/// Цвет вкладки. Невалидный цвет — цвет не задан (вкладка в цвете темы).
	QColor color() const;
	void setColor(const QColor& color);

	/// Тип документа (xtype), которым он создан: используется группировкой
	/// вкладок и подсказками. Пустая строка — тип не задан.
	QString xtype() const;
	void setXtype(const QString& xtype);

	/// Полное имя класса документа (`WidgetTypeHandler::className`): по нему
	/// тема задаёт цвет вкладки. Пустая строка — класс не задан.
	QString className() const;
	void setClassName(const QString& className);

	/// Имя переменной контекста, значение которой показывается подсказкой
	/// вкладки (из метаданных типа документа). Пустая строка — подсказки нет.
	QString tabToolTipVariable() const;
	void setTabToolTipVariable(const QString& variable);

	/// Подсказка вкладки: значение переменной tabToolTipVariable().
	/// Пустая строка — переменная не задана или её значение пусто.
	QString tabToolTip() const;

	/// Контекст документа. Может быть nullptr у документов, созданных
	/// напрямую (например, в тестах или legacy-окнах).
	std::shared_ptr<CommandContext> context() const;
	void setContext(const std::shared_ptr<CommandContext>& context);

	/// Значение переменной контекста. Если переменной нет (ни собственной,
	/// ни унаследованной), она создаётся со значением defaultValue.
	QVariant variable(const QString& name, const QVariant& defaultValue = QVariant());

	/// Записать переменную в контекст документа (false — контекст только для чтения
	/// или отсутствует).
	bool setVariable(const QString& name, const QVariant& value);

	/// true, если переменная видна в контексте (собственная или унаследованная).
	bool hasVariable(const QString& name) const;

	/// Редактор свойств (показывается в группе «Свойства документа»).
	/// Базовая реализация возвращает QWidget с пустым QFormLayout — наследники
	/// дополняют его строками.
	virtual QWidget* createPropertiesEditor(QWidget* parent);

	/// Дополнительные группы свойств документа (например, «Параметры поиска»).
	/// Показываются вложенными в группу «Свойства документа».
	virtual QList<DocumentPropertyGroup> createPropertyGroups(QWidget* parent);

	/// Редактор свойств вкладки (заголовок, значок, цвет, тип, изменённость).
	/// Показывается в отдельной группе «Свойства вкладки».
	virtual QWidget* createTabPropertiesEditor(QWidget* parent);

	/// Открыть панель «Свойства» на этом документе и раскрыть группу
	/// (пустая строка — только показать панель). false — оболочка недоступна.
	bool showProperties(const QString& groupId = QString());

	/// «Обновить» (кнопка ⟳ в панели свойств)
	virtual void refresh() {}

	/// Сохранение (вызывается при закрытии изменённого документа)
	virtual bool save() {
		return true;
	}

	/// Можно ли закрыть документ (по умолчанию спрашивает при isModified())
	virtual bool maybeClose();

signals:
	void titleChanged(const QString& title);
	void glyphChanged(const QString& glyph);
	void modifiedChanged(bool modified);
	void colorChanged(const QColor& color);
	void xtypeChanged(const QString& xtype);
	void tabToolTipChanged();
	void propertiesChanged();   // попросить панель свойств перестроить редактор

protected:
	/// Документу назначен контекст: наследник читает состояние из переменных
	/// и создаёт недостающие значения по умолчанию (см. variable()).
	virtual void onContextAttached() {}

	/// Изменилась собственная переменная контекста документа.
	virtual void onContextVariableChanged(const QString& name, const QVariant& value) {
		Q_UNUSED(name);
		Q_UNUSED(value);
	}

	void closeEvent(QCloseEvent* e) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
