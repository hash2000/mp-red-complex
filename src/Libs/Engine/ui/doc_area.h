#pragma once
#include <QWidget>
#include <memory>

class DocumentWidget;

class DocumentArea : public QWidget {
	Q_OBJECT
public:
	explicit DocumentArea(QWidget* parent = nullptr);
	~DocumentArea() override;

	void addDocument(DocumentWidget* doc);      // забирает владение и активирует
	bool closeDocument(DocumentWidget* doc);    // false — пользователь отменил
	bool closeAll();

	void setCurrentDocument(DocumentWidget* doc);
	DocumentWidget* currentDocument() const;
	QList<DocumentWidget*> documents() const;

signals:
	void documentAdded(DocumentWidget* doc);
	void documentAboutToClose(DocumentWidget* doc);
	/// doc служит только идентификатором: объект уже уничтожен.
	void documentDestroyed(DocumentWidget* doc);
	void currentDocumentChanged(DocumentWidget* doc);   // может быть nullptr

private:
	struct Private;
	std::unique_ptr<Private> d;
};
