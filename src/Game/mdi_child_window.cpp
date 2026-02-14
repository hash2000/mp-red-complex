#include "Game/mdi_child_window.h"
#include "Game/commands/command_context.h"

class MdiChildWindow::Private {
public:
	Private(MdiChildWindow* parent)
		: q(parent) {
	}

	MdiChildWindow* q;
	QString windowId;
};


MdiChildWindow::MdiChildWindow(const QString& id, QWidget* parent)
	: QMdiSubWindow(parent)
	, d(std::make_unique<Private>(this)) {
	d->windowId = id;
}

MdiChildWindow::~MdiChildWindow() = default;

QString MdiChildWindow::windowType() const {
	return "window";
}

QString MdiChildWindow::windowTitle() const {
	return "window";
}

QString MdiChildWindow::windowId() const {
	return d->windowId;
}

QSize MdiChildWindow::windowDefaultSizes() const {
	return QSize(640, 480);
}

bool MdiChildWindow::handleCommand(const QString& commandName,
	const QStringList& args,
	CommandContext* context) {
	Q_UNUSED(commandName);
	Q_UNUSED(args);
	Q_UNUSED(context);
	return false;
}
