#pragma once
#include <QString>
#include <QWidget>
#include <memory>

class MdiChildWindow;
class CommandContext;

class WindowsBuilder {
public:
	WindowsBuilder();
	~WindowsBuilder();

	MdiChildWindow* build(const QString& name, const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent);

private:
	class Private;
	std::unique_ptr<Private> d;
};
