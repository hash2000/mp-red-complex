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
	void setActive(const QString& id); // пустая строка — ни одна не активна

	void setAutoHide(bool on); // true — свернуть в полоску, раскрывать по hover
	bool autoHide() const;

signals:
	void pinToggled(bool pinned);
	void buttonClicked(const QString& id);

protected:
	void enterEvent(QEnterEvent* e) override;
	void leaveEvent(QEvent* e) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
