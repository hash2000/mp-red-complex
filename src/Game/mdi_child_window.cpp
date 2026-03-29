#include "Game/mdi_child_window.h"
#include "Game/commands/command_context.h"
#include <QMdiArea>

class MdiChildWindow::Private {
public:
	Private(MdiChildWindow* parent)
		: q(parent) {
	}

	MdiChildWindow* q;
	QString windowId;
	QPointer<QMdiArea> mdiArea;
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

void MdiChildWindow::setMdiArea(QMdiArea* mdiArea) {
	d->mdiArea = mdiArea;
}

bool MdiChildWindow::NeedCentering() const {
	return false;
}

bool MdiChildWindow::NeedFixedSize() const {
	return false;
}

void MdiChildWindow::setupMdiArea() {
	const QSize windowSize = windowDefaultSizes();

	if (NeedFixedSize()) {
		setFixedSize(windowSize);
	}

	if (!NeedCentering()) {
		return;
	}

	if (!d->mdiArea) {
		return;
	}

	// Для QMdiSubWindow нужно центрировать относительно viewport
	const QSize areaSize = d->mdiArea->viewport()->size();

	// Вычисляем центральную позицию
	int x = (areaSize.width() - windowSize.width()) / 2;
	int y = (areaSize.height() - windowSize.height()) / 2;

	// Убеждаемся, что окно не выйдет за границы
	x = qMax(0, x);
	y = qMax(0, y);

	// Устанавливаем размер и позицию
	resize(windowSize);
	move(x, y);
	
	// Обновляем окно
	update();
}

bool MdiChildWindow::handleCommand(const QString& commandName,
	const QStringList& args,
	CommandContext* context) {
	Q_UNUSED(commandName);
	Q_UNUSED(args);
	Q_UNUSED(context);
	return false;
}
