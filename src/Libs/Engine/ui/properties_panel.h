#pragma once
#include <QWidget>
#include <memory>

class DocumentWidget;

class PropertiesPanel : public QWidget {
	Q_OBJECT
public:
	explicit PropertiesPanel(QWidget* parent = nullptr);
	~PropertiesPanel() override;

	void setDocument(DocumentWidget* doc);
	DocumentWidget* document() const;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
