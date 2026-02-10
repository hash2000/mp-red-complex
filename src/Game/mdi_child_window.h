#pragma once
#include <QWidget>
#include <QString>
#include <QSize>

class CommandContext;

class MdiChildWindow : public QWidget {
	Q_OBJECT
public:
	explicit MdiChildWindow(const QString& id, QWidget* parent = nullptr);
	~MdiChildWindow() override;

	virtual QString windowType() const;
	virtual QString windowTitle() const;
	QString windowId() const;

	virtual QSize windowDefaultSizes() const;

	virtual bool handleCommand(const QString& commandName,
		const QStringList& args,
		class CommandContext* context);

signals:
	void requestClose();

private:
	class Private;
	std::unique_ptr<Private> d;
};
