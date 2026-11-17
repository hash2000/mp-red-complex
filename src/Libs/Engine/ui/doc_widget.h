#pragma once
#include <QWidget>
#include <memory>

class CommandContext;

/// Базовый класс для всех виджетов-документов.
/// Каждому DocumentWidget автоматически соответствует вкладка слева.
/// Документ связан с остальными документами через свой CommandContext
/// (`.documents.{id}`), поэтому изменение свойств и вызовы видны соседям.
class DocumentWidget : public QWidget {
	Q_OBJECT
public:
	explicit DocumentWidget(QWidget* parent = nullptr);
	~DocumentWidget() override;

	QString title() const;
	void setTitle(const QString& title);

	QString glyph() const;               // UTF-8 значок вкладки
	void setGlyph(const QString& glyph);

	bool isModified() const;
	void setModified(bool modified);

	/// Контекст документа. Может быть nullptr у документов, созданных
	/// напрямую (например, в тестах или legacy-окнах).
	std::shared_ptr<CommandContext> context() const;
	void setContext(const std::shared_ptr<CommandContext>& context);

	/// Редактор свойств (показывается в панели «Свойства»).
	/// Базовая реализация возвращает QWidget с QFormLayout — наследники
	/// могут дополнять его строками.
	virtual QWidget* createPropertiesEditor(QWidget* parent);

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
	void propertiesChanged();   // попросить панель свойств перестроить редактор

protected:
	void closeEvent(QCloseEvent* e) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
