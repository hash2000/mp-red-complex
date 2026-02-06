#pragma once
#include <QWidget>
#include <QString>
#include <QSize>

class CommandContext;

class MdiChildWindow : public QWidget {
	Q_OBJECT
public:
	explicit MdiChildWindow(QWidget* parent = nullptr);
	~MdiChildWindow() override;

	virtual QString windowType() const;
	virtual QString windowTitle() const;
	QString windowId() const;
	void setWindowId(const QString& id);

	virtual QSize windowDefaultSizes() const;

	virtual bool handleCommand(const QString& commandName,
		const QStringList& args,
		class CommandContext* context);

signals:
	void requestClose();

protected:
	QString m_windowId;
};
