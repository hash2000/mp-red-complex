#pragma once
#include <QWidget>
#include <memory>

class DocumentWidget;

class DocumentTabsPanel : public QWidget {
	Q_OBJECT
public:
	explicit DocumentTabsPanel(QWidget* parent = nullptr);
	~DocumentTabsPanel() override;

	void addDocument(DocumentWidget* doc);
	void removeDocument(DocumentWidget* doc);
	void setActiveDocument(DocumentWidget* doc);

signals:
	void activateRequested(DocumentWidget* doc);
	void closeRequested(DocumentWidget* doc);
	void propertiesRequested(DocumentWidget* doc);
	void newDocumentRequested();

private:
	struct Private;
	std::unique_ptr<Private> d;
};
