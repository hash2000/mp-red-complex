#pragma once
#include <QWidget>
#include <memory>

/// Базовый класс для всех виджетов центральной области.
/// Каждому DocumentWidget автоматически соответствует вкладка слева.
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
