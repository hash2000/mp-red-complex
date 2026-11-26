#pragma once
#include <QWidget>
#include <memory>

class QEnterEvent;

class BottomPanel : public QWidget {
	Q_OBJECT
public:
	explicit BottomPanel(QWidget* parent = nullptr);
	~BottomPanel() override;

	void addPanel(const QString& id, const QString& glyph, const QString& title, QWidget* panel);
	bool removePanel(const QString& id);
	void setCurrentPanel(const QString& id);
	QWidget* panel(const QString& id) const;

	/// Свернуть: скрыть шапку и содержимое, оставить полоску вкладок.
	void setCollapsed(bool collapsed);
	bool isCollapsed() const;

	/// Автоскрытие (yaml: autohide): содержимое панели прячется, остаётся
	/// горизонтальная полоска вкладок с кнопкой закрепления; при наведении
	/// панель раскрывается.
	void setAutoHide(bool autoHide);
	bool autoHide() const;

	/// false — кнопка сворачивания скрыта, сворачивание кликом по активной
	/// вкладке запрещено (yaml: collapsable).
	void setCollapsable(bool collapsable);
	bool isCollapsable() const;

	/// false — кнопка «крестик» скрыта (yaml: closable).
	void setClosable(bool closable);
	bool isClosable() const;

	/// Высота свёрнутой панели — полоска вкладок.
	int collapsedHeight() const;

signals:
	void collapsedChanged(bool collapsed);
	void closeRequested();

protected:
	void enterEvent(QEnterEvent* e) override;
	void leaveEvent(QEvent* e) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
