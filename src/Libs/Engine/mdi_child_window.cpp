#include "Libs/Engine/mdi_child_window.h"
#include <QMdiArea>

class MdiChildWindow::Private {
public:
	Private(MdiChildWindow* parent)
		: q(parent) {
	}

	MdiChildWindow* q;
	QString windowId;
	QPointer<QMdiArea> mdiArea;
	std::shared_ptr<CommandContext> currentContext;
};


MdiChildWindow::MdiChildWindow(const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent)
	: QMdiSubWindow(parent)
	, d(std::make_unique<Private>(this)) {
	d->windowId = id;
	d->currentContext = context;
}

MdiChildWindow::~MdiChildWindow() = default;

std::shared_ptr<CommandContext> MdiChildWindow::currentContext() const {
	return d->currentContext;
}

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

bool MdiChildWindow::handleCommand(const std::shared_ptr<Instruction> cmd) {
	return false;
}
