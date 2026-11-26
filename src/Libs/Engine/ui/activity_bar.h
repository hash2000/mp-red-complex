#pragma once
#include <QWidget>
#include <memory>

class ActivityBar : public QWidget {
	Q_OBJECT
public:
	enum class Position {
		Top, Bottom
	};

	explicit ActivityBar(QWidget* parent = nullptr);
	~ActivityBar() override;

	void addButton(const QString& id, const QString& glyph, const QString& toolTip,
		Position pos = Position::Top, bool checkable = true);
	void removeButton(const QString& id);
	void setActive(const QString& id); // пустая строка — ни одна не активна

	/// Кнопка сворачивания панели кнопок (yaml: sidebar collapsable).
	/// false — кнопка скрыта, панель кнопок остаётся развёрнутой.
	void setPinVisible(bool visible);
	bool isPinVisible() const;

	void setAutoHide(bool on); // true — свернуть в полоску, раскрывать по hover
	bool autoHide() const;

signals:
	void pinToggled(bool pinned);
	/// Панель кнопок развернулась/свернулась: и по кнопке, и по autohide
	/// (наведение/уход мыши).
	void expandedChanged(bool expanded);
	void buttonClicked(const QString& id);

protected:
	void enterEvent(QEnterEvent* e) override;
	void leaveEvent(QEvent* e) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
