#pragma once
#include <QMdiSubWindow>
#include <QWidget>
#include <QString>
#include <QSize>
#include <QPointer>

class CommandContext;
class QMdiArea;

class MdiChildWindow : public QMdiSubWindow {
	Q_OBJECT
public:
	explicit MdiChildWindow(const QString& id, QWidget* parent = nullptr);
	~MdiChildWindow() override;

	virtual QString windowType() const;
	virtual QString windowTitle() const;
	QString windowId() const;

	virtual QSize windowDefaultSizes() const;

	/// Установить MDI area для центрирования
	void setMdiArea(QMdiArea* mdiArea);

	/// Центрировать окно относительно родительского MDI area
	void centerInMdiArea();

	virtual bool handleCommand(const QString& commandName,
		const QStringList& args,
		class CommandContext* context);

private:
	class Private;
	std::unique_ptr<Private> d;
};
