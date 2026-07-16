#pragma once
#include <QString>
#include <QWidget>
#include <memory>

class MdiChildWindow;

class WindowsBuilder {
public:
	WindowsBuilder();
	~WindowsBuilder();

	MdiChildWindow* build(const QString& name, const QString& id, QWidget* parent);

private:
	class Private;
	std::unique_ptr<Private> d;
};
