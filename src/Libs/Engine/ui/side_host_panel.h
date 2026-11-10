#pragma once
#include <QWidget>
#include <memory>

class SideHostPanel : public QWidget {
	Q_OBJECT
public:
	explicit SideHostPanel(QWidget* parent = nullptr);
	~SideHostPanel() override;

	void addPanel(const QString& id, const QString& title, QWidget* panel);
	void setCurrentPanel(const QString& id);
	QString currentPanel() const;
	QWidget* panel(const QString& id) const;

	void setPinned(bool pinned);
	bool isPinned() const;

signals:
	void pinToggled(bool pinned);
	void closeRequested();

private:
	struct Private;
	std::unique_ptr<Private> d;
};
