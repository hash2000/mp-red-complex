#include "Game/mdi_child_window.h"
#include "Game/commands/command_context.h"

MdiChildWindow::MdiChildWindow(QWidget* parent)
	: QWidget(parent) {
}

MdiChildWindow::~MdiChildWindow() = default;

QString MdiChildWindow::windowType() const {
	return "window";
}

QString MdiChildWindow::windowId() const {
	return m_windowId;
}

void MdiChildWindow::setWindowId(const QString& id) {
	m_windowId = id;
}

bool MdiChildWindow::handleCommand(const QString& commandName,
	const QStringList& args,
	CommandContext* context) {
	Q_UNUSED(commandName);
	Q_UNUSED(args);
	Q_UNUSED(context);
	return false;
}
