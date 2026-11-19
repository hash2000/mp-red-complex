#pragma once
#include <QWidget>
#include <memory>

class BottomPanel : public QWidget {
	Q_OBJECT
public:
	explicit BottomPanel(QWidget* parent = nullptr);
	~BottomPanel() override;

	void addPanel(const QString& id, const QString& glyph, const QString& title, QWidget* panel);
	bool removePanel(const QString& id);
	void setCurrentPanel(const QString& id);
	QWidget* panel(const QString& id) const;

	void setCollapsed(bool collapsed);   // свернуть до полоски с вкладками
	bool isCollapsed() const;

signals:
	void collapsedChanged(bool collapsed);
	void closeRequested();

private:
	struct Private;
	std::unique_ptr<Private> d;
};
